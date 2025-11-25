#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <hardware/memory/pmm.h>
#include <hardware/memory/paging.h>

#include <system/multitasking/tasksched.h>
#include <system/exec/elf_loader.h>
#include <system/exec/user.h>
#include <system/exec/elf64/ehdr64.h>
#include <system/exec/elf64/phdr64.h>

Thread* current_thread = NULL;
int scheduler_running = 0;

Thread* thread_list = NULL;
Process* process_list = NULL;
static uint64_t next_tid = 1;
static uint64_t next_pid = 1;

extern uintptr_t hhdm;
extern uint64_t create_user_address_space(void);
extern void mapPage_in_pml4(uint64_t pml4_virt, void *virt, void *phys, uint64_t flags);

extern uint8_t *kernel_stack_top;
extern uint8_t kernel_stack[];

extern uint16_t gdt_user_data_selector;
extern uint16_t gdt_user_code_selector;

static void idle_loop(void) {
    for (;;) {
        __asm__ volatile("hlt");
    }
}

void task_trampoline(void) {
    extern void enter_userspace_from_task(Thread* task);
    extern uint64_t virt_to_phys_in_pml4(uint64_t pml4_phys, void *virt);
    extern void debug_walk_paging(uint64_t pml4_phys, uint64_t virt);
    
    uint64_t phys = virt_to_phys_in_pml4(current_thread->context.cr3, (void*)current_thread->context.user_rip);
    if (!phys) {
        printf("[ ERROR ] Entry point %#llx is NOT MAPPED in CR3 %#llx!\n",
               current_thread->context.user_rip, current_thread->context.cr3);
        debug_walk_paging(current_thread->context.cr3, current_thread->context.user_rip);
        for(;;) __asm__ volatile("hlt");
    }
    
    if (current_thread->context.cr3) {
        __asm__ volatile("mov %0, %%cr3" : : "r"(current_thread->context.cr3) : "memory");
    }
    
    current_thread->in_userspace = 1;
    
    enter_userspace_from_task(current_thread);
    
    for (;;) __asm__ volatile("hlt");
}

void scheduler_init(void) {
    static Thread idle;
    memset(&idle, 0, sizeof(idle));
    idle.tid = 0;
    idle.state = THREAD_STATE_RUNNING;
    idle.remaining_time = 0;
    idle.kernel_stack = kernel_stack;
    idle.kernel_stack_top = kernel_stack_top;
    idle.priority = THREAD_PRIORITY_LOW;
    
    idle.context.rip = (uint64_t)idle_loop;
    idle.context.rsp = (uint64_t)kernel_stack_top;
    idle.context.rflags = 0x202;
    idle.next = NULL;
    
    thread_list = &idle;
    current_thread = &idle;
}

Thread* schedule(void) {
    Thread* start = current_thread ? current_thread->next : thread_list;
    if (!start) start = thread_list;
    
    Thread* t = start;
    int safety = 0;
    do {
        if (!t) {
            printf("[ SCHEDULER ERROR ] NULL thread in list!\n");
            return current_thread;
        }
        
        if ((uint64_t)t < 0xFFFF800000000000ULL) {
            printf("[ SCHEDULER ERROR ] Invalid thread pointer %p (too low)!\n", t);
            printf("[ SCHEDULER ] current_thread=%p, thread_list=%p\n", current_thread, thread_list);
            if (current_thread) {
                printf("[ SCHEDULER ] current_thread->next=%p, tid=%llu\n", 
                       current_thread->next, current_thread->tid);
            }
            for(;;) __asm__ volatile("cli; hlt");
        }
        
        if (t->tid > 1000) {
            printf("[ SCHEDULER ERROR ] Bogus TID=%llu, list corrupted!\n", t->tid);
            printf("[ SCHEDULER ] Thread pointer: %p\n", t);
            printf("[ SCHEDULER ] Thread->next: %p\n", t->next);
            return current_thread;
        }
        
        if (t->state == THREAD_STATE_READY) {
            return t;
        }
        t = t->next;
        if (!t) t = thread_list;
        
        if (++safety > 10) {
            printf("[ SCHEDULER ERROR ] Infinite loop detected!\n");
            return current_thread;
        }
    } while (t != start);
    
    return current_thread;
}

Process* create_process(void* elf_data) {
    Process* proc = malloc(sizeof(Process));
    memset(proc, 0, sizeof(Process));
    
    proc->pid = next_pid++;
    proc->thread_count = 0;
    proc->thread_list = NULL;
    
    proc->cr3 = create_user_address_space();
    
    ELF64_Hdr_t* eh = (ELF64_Hdr_t*)elf_data;
    ELF64_Phdr_t* ph = (ELF64_Phdr_t*)((uint8_t*)elf_data + eh->e_phoff);
    
    for (int i = 0; i < eh->e_phnum; i++) {
        if (ph[i].p_type != ELF_SEGMENT_TYPE_LOAD) continue;

        uint64_t vaddr = ph[i].p_vaddr;
        uint64_t memsz = ph[i].p_memsz;
        uint64_t filesz = ph[i].p_filesz;
        uint64_t offset = ph[i].p_offset;
        
        uint64_t page_base = vaddr & ~0xFFFULL;
        uint64_t page_off = vaddr & 0xFFFULL;
        uint64_t total_size = page_off + memsz;
        uint64_t pages_needed = (total_size + PAGE_SIZE - 1) / PAGE_SIZE;

        uintptr_t phys_base = alloc_pages(pages_needed);
        
        uint64_t flags = PG_PRESENT | PG_USER;
        if (ph[i].p_flags & ELF_SEGMENT_FLAG_WRITABLE)
            flags |= PG_WRITABLE;
        if (!(ph[i].p_flags & ELF_SEGMENT_FLAG_EXECUTABLE))
            flags |= PG_NX;
        
        for (uint64_t p = 0; p < pages_needed; ++p) {
            void* virt_addr = (void*)(page_base + p * PAGE_SIZE);
            void* phys_addr = (void*)(phys_base + p * PAGE_SIZE);
            mapPage_in_pml4(proc->cr3, virt_addr, phys_addr, flags);
        }
        
        uint8_t* kernel_mapped_addr = (uint8_t*)(phys_base + hhdm);
        memcpy(kernel_mapped_addr + page_off, (uint8_t*)elf_data + offset, filesz);
        memset(kernel_mapped_addr + page_off + filesz, 0, memsz - filesz);
    }
    
    Thread* main_thread = malloc(sizeof(Thread));
    
    main_thread->tid = next_tid++;
    main_thread->state = THREAD_STATE_READY;
    main_thread->process = proc;
    main_thread->priority = THREAD_PRIORITY_MEDIUM;
    main_thread->remaining_time = 100;
    
    uintptr_t kphys = alloc_pages(4);
    main_thread->kernel_stack = (void*)(kphys + hhdm);
    main_thread->kernel_stack_top = (void*)((uint8_t*)main_thread->kernel_stack + 4*PAGE_SIZE);

    uintptr_t stack_phys = alloc_pages(USER_STACK_PAGES);
    
    for (uint64_t p = 0; p < USER_STACK_PAGES; p++) {
        void* virt = (void*)(USER_STACK_TOP - (p+1) * PAGE_SIZE);
        void* phys = (void*)(stack_phys + p * PAGE_SIZE);
        mapPage_in_pml4(proc->cr3, virt, phys, PG_PRESENT|PG_WRITABLE|PG_USER|PG_NX);
    }
    
    main_thread->context.cr3 = proc->cr3;
    main_thread->context.rip = (uint64_t)task_trampoline;
    main_thread->context.rsp = (uint64_t)main_thread->kernel_stack_top;
    main_thread->context.user_rip = eh->e_entry;
    main_thread->context.user_rsp = USER_STACK_TOP;
    main_thread->context.rflags = 0x202;
    main_thread->in_userspace = 0;
    
    proc->main_thread = main_thread;
    proc->thread_list = main_thread;
    proc->thread_count = 1;
    main_thread->next_in_process = NULL;
    
    main_thread->next = thread_list;
    thread_list = main_thread;
    
    proc->next = process_list;
    process_list = proc;
    
    return proc;
}

void finalize_thread_list(void) {
    if (!thread_list) return;
    
    Thread* head = thread_list;
    Thread* last = thread_list;
    
    while (last->next != NULL) {
        last = last->next;
    }
    
    last->next = head;
    
    __asm__ volatile("mfence" ::: "memory");
}

Thread* create_thread(Process* proc, void* user_stack) {
    Thread* thread = malloc(sizeof(Thread));
    memset(thread, 0, sizeof(Thread));
    
    thread->tid = next_tid++;
    thread->state = THREAD_STATE_READY;
    thread->process = proc;
    thread->priority = THREAD_PRIORITY_MEDIUM;
    
    uintptr_t kphys = alloc_pages(4);
    thread->kernel_stack = (void*)(kphys + hhdm);
    thread->kernel_stack_top = (void*)((uint8_t*)thread->kernel_stack + 4*PAGE_SIZE);
    
    thread->context.cr3 = proc->cr3;
    
    thread->context.user_rsp = (uint64_t)user_stack;
    //thread->context.user_rip = /* caller sets this */;
    
    thread->next_in_process = proc->thread_list;
    proc->thread_list = thread;
    proc->thread_count++;
    
    __asm__ volatile("cli");
    
    if (!thread_list) {
        thread->next = thread;
        thread_list = thread;
    } else {
        Thread* tail = thread_list;
        while (tail->next != thread_list) {
            tail = tail->next;
        }
        
        thread->next = thread_list;
        tail->next = thread;
        thread_list = thread;
    }
    
    __asm__ volatile("sti");
    
    return thread;
}

void terminate_process(Process* proc, int exit_code) {
    Thread* t = proc->thread_list;
    while (t) {
        t->state = THREAD_STATE_DONE;
        t = t->next_in_process;
    }
    
    printf("[ PROCESS ] Process %llu exited with exit code %d.\n",
           (unsigned long long)proc->pid, exit_code);
}