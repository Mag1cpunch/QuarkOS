#include <string.h>

#include <hardware/memory/gdt.h>
#include <hardware/memory/tss.h>

struct Tss tss;
extern uint16_t gdt_tss_selector;

void tss_init_gdt(void *tss_entry)
{
    uint64_t tss_base = (uint64_t)&tss;
    uint64_t tss_limit = sizeof(tss) - 1;

    struct TSSDescriptor {
        uint16_t limit_low;
        uint16_t base_low;
        uint8_t  base_mid;
        uint8_t  type_attr;
        uint8_t  limit_high_flags;
        uint8_t  base_high;
        uint32_t base_upper;
        uint32_t reserved;
    } __attribute__((packed));

    struct TSSDescriptor *desc = (struct TSSDescriptor *)tss_entry;

    desc->limit_low = tss_limit & 0xFFFF;
    desc->base_low  = tss_base & 0xFFFF;
    desc->base_mid  = (tss_base >> 16) & 0xFF;
    desc->type_attr = 0x89;
    desc->limit_high_flags = ((tss_limit >> 16) & 0x0F);
    desc->base_high = (tss_base >> 24) & 0xFF;
    desc->base_upper = (tss_base >> 32) & 0xFFFFFFFF;
    desc->reserved = 0;
}

static uint8_t interrupt_stack[8192] __attribute__((aligned(16)));

void tss_init()
{
    memset(&tss, 0, sizeof(tss));
    tss.rsp0 = (uint64_t)interrupt_stack + sizeof(interrupt_stack);
}