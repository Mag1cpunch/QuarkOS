#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <arch/x86_64/isr.h>

#define IA32_EFER_MSR 0xC0000080
#define EFER_SCE (1ULL << 0)
#define IA32_STAR_MSR 0xC0000081
#define IA32_LSTAR_MSR 0xC0000082
#define IA32_FMASK_MSR 0xC0000084

void syscall_init(void);
void syscall_handler(struct InterruptFrame* frame);

#endif // SYSCALLS_H