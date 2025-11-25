#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>

#include <arch/x86_64/syscalls.h>
#include <arch/x86_64/isr.h>
#include <arch/x86_64/cpu.h>

#include <hardware/memory/gdt.h>
#include <hardware/memory/paging.h>
#include <hardware/memory/tss.h>

#include <system/term.h>
#include <system/multitasking/spinlock.h>
#include <system/multitasking/tasksched.h>

#define SYSCALL_COUNT 1024

extern uint16_t gdt_kernel_code_selector;
extern uint16_t gdt_user_data_selector;
extern uint16_t gdt_user_code_selector;
extern void context_switch(ThreadContext *old, ThreadContext *new);
extern Thread* current_thread;
extern uintptr_t hhdm;

static void sys_write(uint64_t fd, const char* buf, uint64_t len) {
    if (fd != 1 && fd != 2) {
        return;
    }
    if (fd == 2)
        printf("[ ERROR ] ");

    for (uint64_t i = 0; i < len; i++) {
        uint64_t vaddr = (uint64_t)buf + i;
        uint64_t phys = virt_to_phys_in_pml4(current_thread->context.cr3, (void*)vaddr);
        
        if (phys == 0) {
            printf("\n[ SYSCALL ERROR ] Bad userspace address: %#llx\n", vaddr);
            return;
        }
        
        char* kernel_ptr = (char*)(phys + hhdm);
        kputchar(*kernel_ptr);
    }
}

static uint64_t sys_fork(struct InterruptFrame* frame, uint64_t flags __attribute__((unused)), void* child_stack) {
    Process* proc = current_thread->process;
    
    void* user_stack = child_stack;
    if (!user_stack) {
        user_stack = (void*)frame->rsp;
    }
    
    Thread* child = create_thread(proc, user_stack);
    
    child->context.user_rip = frame->rip;
    child->context.user_rsp = child_stack ? (uint64_t)child_stack : frame->rsp;
    child->context.rflags = frame->rflags;
    
    child->context.rax = 0;
    child->context.rcx = frame->rcx;
    child->context.rdx = frame->rdx;
    child->context.rsi = frame->rsi;
    child->context.rdi = frame->rdi;
    child->context.r8  = frame->r8;
    child->context.r9  = frame->r9;
    child->context.r10 = frame->r10;
    child->context.r11 = frame->r11;
    
    child->context.rbx = 0;
    child->context.rbp = 0;
    child->context.r12 = 0;
    child->context.r13 = 0;
    child->context.r14 = 0;
    child->context.r15 = 0;
    
    child->state = THREAD_STATE_READY;
    child->in_userspace = 1;
    
    child->context.rip = (uint64_t)task_trampoline;
    child->context.rsp = (uint64_t)child->kernel_stack_top;
    
    return child->tid;
}

static void sys_exit(int code) __attribute__((noreturn));

static void sys_exit(int code) { 
    current_thread->state = THREAD_STATE_DONE;
    printf("[ EXIT ] Thread TID %llu exited with code %d\n", 
           current_thread->tid, code);
    
    releaseLock();
    
    __asm__ volatile("cli");
    
    extern Thread* schedule(void);
    Thread *next = schedule();
    
    if (!next || next == current_thread || next->state == THREAD_STATE_DONE) {
        printf("[ KERNEL ] All threads completed. System halting.\n");
        for(;;) __asm__ volatile("cli; hlt");
    }
    
    next->state = THREAD_STATE_RUNNING;
    Thread *old = current_thread;
    current_thread = next;
    
    if (next->remaining_time == 0) {
        next->remaining_time = BASE_TIME_QUANTUM * next->priority;
    }
    
    extern struct Tss tss;
    tss.rsp0 = (uint64_t)next->kernel_stack_top;
    
    extern void exit_to_thread(Thread* thread) __attribute__((noreturn));
    exit_to_thread(next);
}

static uint64_t sys_reboot() {
    return current_thread->tid;
}

static void* syscall_handlers[SYSCALL_COUNT];

extern void syscall_entry_fast(void);

void syscall_init(void) {
    for (size_t i = 0; i < SYSCALL_COUNT; i++) {
        syscall_handlers[i] = NULL;
    }

    syscall_handlers[1] = (void*)sys_write;
    syscall_handlers[57] = (void*)sys_fork;
    syscall_handlers[60] = (void*)sys_exit;
    syscall_handlers[88] = (void*)sys_reboot;

    uint64_t efer = rdmsr(IA32_EFER_MSR);
    printf("[ SYSCALLS ] EFER before: %#llx\n", efer);
    efer |= (1 << 0);  // SCE = bit 0
    wrmsr(IA32_EFER_MSR, efer);
    printf("[ SYSCALLS ] EFER after: %#llx\n", rdmsr(IA32_EFER_MSR));
    wrmsr(IA32_LSTAR_MSR, (uint64_t)syscall_entry_fast);
    wrmsr(IA32_FMASK_MSR, 0x202);
    
    uint64_t star = 0;
    star |= ((uint64_t)gdt_kernel_code_selector) << 32;
    
    uint16_t user_base_selector = (gdt_user_data_selector & ~3) - 8;
    star |= ((uint64_t)user_base_selector) << 48;

    wrmsr(IA32_STAR_MSR, star);

    printf("[ SYSCALLS ] Initialized. STAR: 0x%llx (User Base: 0x%x)\n", star, user_base_selector);
}

void syscall_handler(struct InterruptFrame* frame) {
    acquireLock();
    uint64_t syscall_number = frame->rax;

    if (syscall_number < SYSCALL_COUNT && syscall_handlers[syscall_number]) {
        uint64_t ret;
        
        if (syscall_number == 60) {
            sys_exit((int)frame->rdi);
            __builtin_unreachable();
        }
        
        if (syscall_number == 57) {
            ret = sys_fork(frame, frame->rdi, (void*)frame->rsi);
        } else {
            typedef uint64_t (*syscall_func_t)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
            syscall_func_t handler = (syscall_func_t)syscall_handlers[syscall_number];
            ret = handler(frame->rdi, frame->rsi, frame->rdx, frame->r10, frame->r8, frame->r9);
        }
        
        frame->rax = ret;
    } else {
        printf("[ ERROR ] Unknown syscall number: %llu from TID %llu\n", 
               syscall_number, current_thread->tid);
        terminate_process(current_thread->process, -1);
        releaseLock();
        
        while(1) {
            __asm__ volatile("sti; hlt; cli");
        }
        __builtin_unreachable();
    }
    releaseLock();
}