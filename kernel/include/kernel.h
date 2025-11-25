#ifndef KERNEL_H
#define KERNEL_H

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#include <stdio.h>
#include <stdint.h>

#include <arch/x86_64/cpu.h>

static void panic(const char *msg) {
    printf("[ Kernel Panic ] %s", msg);
    hcf();
}

#endif