#ifndef GDT_H
#define GDT_H

#define GDT_KERNEL_CODE 0x08
#define GDT_KERNEL_DATA 0x10

#include <stdint.h>

struct __attribute__((packed)) GdtEntry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  gran;
    uint8_t  base_high;
};

struct __attribute__((packed)) Gdtr {
    uint16_t limit;
    uint64_t base;
};

#define GDT_MAX_ENTRIES 16

extern struct GdtEntry gdt[GDT_MAX_ENTRIES];
void gdt_init_from_limine(void);


extern uint16_t gdt_user_code_selector;
extern uint16_t gdt_user_data_selector;
extern uint16_t gdt_tss_selector;
extern uint16_t gdt_kernel_code_selector;
extern uint16_t gdt_kernel_data_selector;

#endif