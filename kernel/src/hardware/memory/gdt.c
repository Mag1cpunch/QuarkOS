#include <string.h>
#include <stdio.h>

#include <hardware/memory/gdt.h>
#include <hardware/memory/tss.h>

struct GdtEntry gdt[GDT_MAX_ENTRIES];
static struct Gdtr gdtr;

uint16_t gdt_user_data_selector;
uint16_t gdt_user_code_selector;
uint16_t gdt_tss_selector;
uint16_t gdt_kernel_code_selector;
uint16_t gdt_kernel_data_selector;

static void gdt_set_entry(int i, uint32_t base, uint32_t limit,
                          uint8_t access, uint8_t flags)
{
    gdt[i].limit_low = limit & 0xFFFF;
    gdt[i].base_low  = base & 0xFFFF;
    gdt[i].base_mid  = (base >> 16) & 0xFF;
    gdt[i].access    = access;
    gdt[i].gran      = ((limit >> 16) & 0x0F) | (flags & 0xF0);
    gdt[i].base_high = (base >> 24) & 0xFF;
}

void gdt_init_from_limine(void)
{
    struct Gdtr old_gdtr;
    asm volatile ("sgdt %0" : "=m"(old_gdtr));

    size_t old_entries = (old_gdtr.limit + 1) / 8;

    uint16_t kernel_cs;
    asm volatile ("mov %%cs, %0" : "=r"(kernel_cs));
    kernel_cs &= ~0x3;
    gdt_kernel_code_selector = kernel_cs;

    if (old_entries > GDT_MAX_ENTRIES - 4) {
        printf("[ GDT ERROR ] Not enough space in local GDT\n");
        for (;;);
    }

    memcpy(&gdt[0], (void*)old_gdtr.base,
           old_entries * sizeof(struct GdtEntry));

    size_t idx = old_entries;

    const size_t KERNEL_CODE_IDX = 5;
    const size_t KERNEL_DATA_IDX = 6;

    size_t USER_DATA_IDX = idx++;
    size_t USER_CODE_IDX = idx++;
    size_t TSS_IDX       = idx;
    idx += 2;
    
    // user data
    gdt_set_entry(USER_DATA_IDX, 0, 0xFFFFFFFF,
                  0b11110010,        // 0xF2: P=1, DPL=3, S=1, type=0010 (write-data)
                  0b10000000);       // 0x80: G=1, D=0, L=0

    // user code
    gdt_set_entry(USER_CODE_IDX, 0, 0xFFFFFFFF,
                  0b11111010,        // 0xFA: P=1, DPL=3, S=1, type=1010 (code)
                  0b10100000);       // 0xA0: G=1, D=0, L=1
    
    gdt_user_data_selector = (USER_DATA_IDX << 3) | 3;
    gdt_user_code_selector = (USER_CODE_IDX << 3) | 3;
    gdt_tss_selector       = (TSS_IDX << 3);

    printf("[ GDT ] User Data Selector: %#x\n", gdt_user_data_selector);
    printf("[ GDT ] User Code Selector: %#x\n", gdt_user_code_selector);
    printf("[ GDT ] TSS Selector: %#x\n", gdt_tss_selector);

    tss_init_gdt(&gdt[TSS_IDX]);

    gdtr.limit = (idx * sizeof(struct GdtEntry)) - 1;
    gdtr.base  = (uint64_t)gdt;

    asm volatile ("lgdt %0" : : "m"(gdtr));
    
    asm volatile (
        "pushq %[cs]\n\t"
        "lea 1f(%%rip), %%rax\n\t"
        "pushq %%rax\n\t"
        "lretq\n\t"
        "1:\n\t"
        :
        : [cs]"r"((uint64_t)kernel_cs)
        : "rax"
    );
    
    uint16_t kernel_ds = kernel_cs + 8;
    gdt_kernel_data_selector = kernel_ds;
    asm volatile (
        "mov %[ds], %%rax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%ss\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"
        :
        : [ds]"r"((uint64_t)kernel_ds)
        : "rax"
    );
}
