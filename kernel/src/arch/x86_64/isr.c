#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <arch/x86_64/isr.h>
#include <arch/x86_64/cpu.h>
#include <arch/x86_64/pic.h>
#include <arch/x86_64/apic/apic.h>
#include <arch/x86_64/pic.h>

#include <hardware/memory/paging.h>

#include <system/flanterm.h>
#include <system/flanterm_backends/fb.h>
#include <system/multitasking/tasksched.h>

void (*interrupt_handlers[256]) (InterruptFrame* frame);

extern void context_switch(Thread* old_thread, Thread* new_thread);

static inline uint64_t getCR2(void)
{
	uint64_t val;
	__asm__ volatile ( "mov %%cr2, %0" : "=r"(val) );
    return val;
}

static void showRegisters(InterruptFrame* frame) {
    printf("Registers:\n");
    printf("RAX: %#zx\n", frame->rax);
    printf("RCX: %#zx\n", frame->rcx);
    printf("RDX: %#zx\n", frame->rdx);
    printf("RDI: %#zx\n", frame->rdi);
    printf("RSI: %#zx\n", frame->rsi);
    printf("RSP: %#zx\n", frame->rsp);
    printf("R8: %#zx\n", frame->r8);
    printf("R9: %#zx\n", frame->r9);
    printf("R10: %#zx\n", frame->r10);
    printf("R11: %#zx\n", frame->r11);
    printf("RFLAGS: %#zx\n", frame->rflags);
    printf("CS: %#zx\n", frame->cs);
    printf("RIP: %#zx\n", frame->rip);
}

extern Thread* current_thread;

void exceptionHandler(InterruptFrame* frame) {
    if (current_thread && current_thread->process->pid != 0) {
        printf("\n=== PROCESS EXCEPTION ===\n");
        printf("Process PID: %llu, Thread TID: %llu\n", 
               current_thread->process->pid, current_thread->tid);
    }
    else
        printf("\n=== CPU EXCEPTION ===\n");
        
	switch (frame->int_no) {
        case 0:
            printf("-- Divide by zero --\n");
            break;
        case 1:
            printf("-- Debug Trap --\n");
            break;
        case 2:
            printf("-- Non-maskable Interrupt --\n");
            break;
        case 3:
            printf("-- Breakpoint --\n");
            break;
        case 4:
            printf("-- Overflow --\n");
            break;
        case 5:
            printf("-- Bound range exceeded --\n");
            break;
        case 6:
            printf("-- Invalid opcode --\n");
            printf("Instruction: %#x", *(&frame->rip));
            break;
        case 7:
            printf("-- Device not available --\n");
            break;
        case 8:
            printf("-- Double fault --\n");
            printf("[ CRITICAL ] This usually means stack overflow or invalid TSS\\n");
            printf("[ INFO ] Error code: %#llx\\n", frame->err_code);
            break;
        case 9:
            printf("-- Coprocessor Segment Overrun --\n");
            break;
        case 10:
            printf("-- Invalid TSS --\n");
            printf("Selector Index: %#zx\n", frame->err_code);
            break;
        case 11:
            printf("-- Segment Not Present --\n");
            break;
        case 12:
            printf("-- Stack-Segment Fault --\n");
            break;
        case 13:
            printf("-- General Protection Fault --\n");
            break;
		case 14:
            printf("-- Page Fault --\n");
			if (frame->err_code & 1) {
				printf("Page protection violation\n");
				//for (;;) 
				//{
				//	__asm__ volatile ("hlt");
				//}
			}
			uint64_t cr2 = getCR2();
			printf("Faulting Address: %#zx\n", cr2);
            break;
        case 16:
            printf("-- X87 Floating-Point Exception --\n");
            break;
        case 17:
            printf("-- Alignment Check --\n");
            break;
        case 18:
            printf("-- Machine Check --\n");
            break;
        case 19:
            printf("-- SIMD Floating-Point Exception --\n");
            break;
        case 20:
            printf("-- Virtualization Exception --\n");
            break;
        case 21:
            printf("-- Control Protection Exception --\n");
            break;
        case 28:
            printf("-- Hypervisor Injection Exception --\n");
            break;
        case 29:
            printf("-- VMM Communication Exception --\n");
            break;
        case 30:
            printf("-- Security Exception --\n");
            break;
        default:
            printf("-- Unhandled Exception or IRQ --\n");
            break;
	}

    printf("Interrupt Number: %d\n", frame->int_no);
    printf("Error Code: %#zx\n", frame->err_code);
    showRegisters(frame);

    if (current_thread) {
        if ((frame->cs & 0x3) == 3 && current_thread && current_thread->process->pid != 0 && (current_thread->state == THREAD_STATE_RUNNING || current_thread->state == THREAD_STATE_BLOCKED)) {
            current_thread->state = THREAD_STATE_DONE;
            printf("Terminating current task due to exception.\n");

            Thread *old = current_thread;
            Thread *next = schedule();

            if (!next || next == old || next->state == THREAD_STATE_DONE) {
                printf("No runnable task after exception, halting.\n");
                hcf();
            }

            if (old->state == THREAD_STATE_RUNNING)
                old->state = THREAD_STATE_READY;

            next->state = THREAD_STATE_RUNNING;
            current_thread = next;

            context_switch(&old->context, &next->context);
            __builtin_unreachable();
        }
    }

    hcf();
}

void registerInterruptHandler(uint8_t interrupt, void (*handler) (InterruptFrame* frame))
{
    interrupt_handlers[interrupt] = handler;
}

extern uint8_t apic_init_done;

void eoi_isr(uint8_t vector) {
    if (apic_init_done) {
        send_eoi_isr(vector, 1, 0);
    } else {
        sendEOIPIC(vector);
    }
}

void irqHandler(InterruptFrame* frame) {
    uint8_t vec = frame->int_no;

    //if (vec != 0xFE && vec != 0x40) {
    //    printf("[IRQ] Received IRQ vector: %u (CS=%#lx, RIP=%#lx)\\n", 
    //           vec, frame->cs, frame->rip);
    //}

    if (interrupt_handlers[vec] != NULL && is_mapped((void*)interrupt_handlers[vec])) {
        interrupt_handlers[vec](frame);
    }
    eoi_isr(vec);
}