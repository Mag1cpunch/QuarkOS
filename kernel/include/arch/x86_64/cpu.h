#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <hardware/devices/io.h>

#define fence() __asm__ volatile ("":::"memory")

static uint64_t frequency_tsc_per_sec;

static void hcf(void) {
    for (;;) {
        asm ("hlt");
    }
}

static inline bool are_interrupts_enabled()
{
    unsigned long flags;
    asm volatile ( "pushf\n\t"
                   "pop %0"
                   : "=g"(flags) );
    return flags & (1 << 9);
}

static inline uint64_t rdtsc(void)
{
    uint32_t low, high;
    asm volatile("rdtsc":"=a"(low),"=d"(high));
    return ((uint64_t)high << 32) | low;
}

static void calibrate_tsc(void)
{
    const uint32_t delay_ms = 10;

    uint64_t t0 = rdtsc();
    for (volatile uint64_t i = 0; i < 3000000ULL; i++) {
        __asm__ volatile("pause");
    }
    uint64_t t1 = rdtsc();

    uint64_t delta = t1 - t0;
    uint64_t freq = (delta * 1000ULL) / delay_ms;

    frequency_tsc_per_sec = freq;
}

static inline void wrmsr(uint64_t msr, uint64_t value)
{
    uint32_t low = value & 0xFFFFFFFF;
    uint32_t high = value >> 32;
    asm volatile (
        "wrmsr"
        :
        : "c"(msr), "a"(low), "d"(high)
    );
}

static inline uint64_t rdmsr(uint64_t msr)
{
    uint32_t low, high;
    asm volatile (
        "rdmsr"
        : "=a"(low), "=d"(high)
        : "c"(msr)
    );
	return ((uint64_t)high << 32) | low;
}

static inline void cpuid(int code, uint32_t* a, uint32_t* d)
{
    asm volatile ( "cpuid" : "=a"(*a), "=d"(*d) : "0"(code) : "ebx", "ecx" );
}

static int has_tsc(void) {
    uint32_t eax = 1, edx = 0;
    cpuid(1, &eax, &edx);
    return (edx >> 4) & 1;
}

static void tsc_sleep(uint64_t ms) {
    if (!has_tsc()) {
        hcf();
    }
    uint64_t tsc_start = rdtsc();
    uint64_t tsc_ticks_target = (frequency_tsc_per_sec / 1000) * 10;
    while ( (rdtsc() - tsc_start) < tsc_ticks_target ) {
        __asm__ volatile("pause");
    }
}

static void reboot() {
    printf("Rebooting...\n");

    IoWrite8(0x64, 0xFE);

    printf("Failed to reboot! Trying triple fault method...\n");
    asm volatile("lidt (0)");
    asm volatile("int $3");
    printf("Failed to reboot! Halting.\n");
    hcf();
}

#endif