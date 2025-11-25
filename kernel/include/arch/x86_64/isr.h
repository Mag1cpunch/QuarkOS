#ifndef ISR_H
#define ISR_H

#include <stdint.h>

typedef struct InterruptFrame {
    // 0  : cr3_saved
    uint64_t cr3_saved;

    // 8..72 : pushed by pushad, in order (top of stack first)
    uint64_t r11, r10, r9, r8;
    uint64_t rsi, rdi, rdx, rcx, rax;

    // 80, 88 : int_no, err_code
    uint64_t int_no;
    uint64_t err_code;

    // 96.. : CPU-pushed stuff
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed)) InterruptFrame;

void exceptionHandler(InterruptFrame* frame);
void irqHandler(InterruptFrame* frame); 
void registerInterruptHandler(uint8_t irq, void (*handler) (InterruptFrame*));

#endif