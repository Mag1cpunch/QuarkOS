#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <limine.h>
#include <kernel.h>

#include <arch/x86_64/idt.h>
#include <arch/x86_64/cpu.h>
#include <arch/x86_64/apic/apic.h>
#include <arch/x86_64/pic.h>

#include <hardware/requests.h>
#include <hardware/memory/pmm.h>
#include <hardware/memory/paging.h>
#include <hardware/memory/heap.h>
#include <hardware/devices/io.h>
#include <hardware/memory/gdt.h>
#include <hardware/memory/tss.h>

#include <system/term.h>
#include <system/exec/user.h>
#include <system/exec/elf64/ehdr64.h>
#include <system/exec/elf64/phdr64.h>
#include <system/exec/elf_enums.h>
#include <system/multitasking/tasksched.h>

__attribute__((aligned(16)))
uint8_t kernel_stack[0x4000];

uint8_t *kernel_stack_top = kernel_stack + sizeof(kernel_stack);

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;

__attribute__((used, section(".limine_requests")))
static volatile struct limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST,
    .revision = 0
};

void kmain(void) {
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        hcf();
    }

    term_init();
    pmm_init();
    initPML4();
    heap_init();
    calibrate_tsc();

    printf("[ KERNEL ] Initializing IDT...\n");
    initIdt();
    printf("[ OK ] Done.\n");

    printf("[ KERNEL ] Initializing PIC...\n");
    PIC_init();
    printf("[ OK ] Done.\n");

    printf("[ KERNEL ] Initializing APIC...\n");
    apic_init();
    printf("[ OK ] Done.\n");

    printf("[ KERNEL ] Starting APIC timer...\n");
    enableAPICTimer(1000);
    printf("[ OK ] Timer active.\n");

    printf("[ KERNEL ] Initializing Syscalls...\n");
    syscall_init();
    printf("[ OK ] Done.\n");

    printf("[ INFO ] Hello, World from QuarkOS kernel!\n");

    printf("[ KERNEL ] Initializing Userspace GDT...\n");
    tss_init();
    gdt_init_from_limine();
    
    extern uint16_t gdt_tss_selector;
    asm volatile("ltr %w0" : : "r"(gdt_tss_selector));
    printf("[ KERNEL ] TSS loaded (selector %#x)\n", gdt_tss_selector);
    
    printf("[ KERNEL ] Initializing Scheduler...\n");
    scheduler_init();

    struct limine_module_response *mresp = module_request.response;
    if (mresp && mresp->module_count > 0) {
        printf("\n=== Modules ===\n\n");
        for (uint64_t i = 0; i < mresp->module_count; i++) {
            struct limine_file *mod = mresp->modules[i];
            printf("[ MODULE %d ] %s at %p, size %llu bytes\n",
                i, mod->path, mod->address, mod->size);
            printf("[ MODULE %d ] Creating process...\n", i);
            Process *p = create_process(mod->address);
            printf("[ MODULE %d ] Process created: PID=%llu\n\n", i, p->pid);
            (void)p;
        }
        printf("===============\n\n");
    }
    else {
        printf("[ KERNEL ] No user-space modules found to run.\n");
    }

    finalize_thread_list();

    printf("[ KERNEL ] Setup complete. Enabling scheduler.\n");

    extern int scheduler_running;
    scheduler_running = 1;
    
    asm volatile ("sti");
    
    printf("[ KERNEL ] Entering idle loop.\n");
    for (;;) {
        asm volatile ("hlt");
    }
}
