#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <limine.h>

#include <arch/x86_64/cpu.h>

#include <hardware/acpi/acpi.h>
#include <hardware/acpi/uacpi/uacpi.h>
#include <hardware/acpi/descriptor_tables/fadt.h>
#include <hardware/requests.h>
#include <hardware/memory/paging.h>
#include <hardware/devices/io.h>

#include <system/multitasking/spinlock.h>
#include <system/multitasking/tasksched.h>

static struct limine_rsdp_response *rsdp_response;
static struct FADT *fadt;

static inline void ensure_mapped_phys_range(uintptr_t phys, size_t size)
{
    uintptr_t start = phys & ~(PAGE_SIZE - 1);
    uintptr_t end   = (phys + size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    for (uintptr_t p = start; p < end; p += PAGE_SIZE) {
        void *v = (void *)virt_addr(p);
        if (!is_mapped(v)) {
            mapPage(v, (void *)p, PG_WRITABLE | PG_NX);
        }
    }
}

static ACPISDTHeader *findFACP_in_XSDT(XSDT *xsdt)
{
    int entries = (xsdt->h.Length - sizeof(xsdt->h)) / 8;
    for (int i = 0; i < entries; i++) {
        uintptr_t phys = (uintptr_t)xsdt->PointerToOtherSDT[i];
        ensure_mapped_phys_range(phys, sizeof(ACPISDTHeader));
        ACPISDTHeader *h = (ACPISDTHeader *)(uintptr_t)virt_addr(phys);
        if (!strncmp(h->Signature, "FACP", 4))
            return h;
    }
    return NULL;
}

static ACPISDTHeader *findFACP_in_RSDT(RSDT *rsdt)
{
    int entries = (rsdt->h.Length - sizeof(rsdt->h)) / 4;
    for (int i = 0; i < entries; i++) {
        uintptr_t phys = (uintptr_t)rsdt->PointerToOtherSDT[i];
        ensure_mapped_phys_range(phys, sizeof(ACPISDTHeader));
        ACPISDTHeader *h = (ACPISDTHeader *)(uintptr_t)virt_addr(phys);
        if (!strncmp(h->Signature, "FACP", 4))
            return h;
    }
    return NULL;
}

static inline void processFADT(struct FADT *fadt_table)
{
    uintptr_t fadt_phys = phys_addr(fadt_table);
    size_t map_size = fadt_table->h.Length > sizeof(struct FADT) ? fadt_table->h.Length : sizeof(struct FADT);
    ensure_mapped_phys_range(fadt_phys, map_size);
    
    fadt = fadt_table;

    if (fadt->AcpiDisable == 0 && fadt->AcpiEnable == 0 && fadt->SMI_CommandPort == 0) {
        printf("[ ACPI ] ACPI already enabled. Skipping...\n");
        return;
    }

    IoWrite8(fadt->SMI_CommandPort, fadt->AcpiEnable);

    while (IoRead16(fadt->PM1aControlBlock) & 1 == 0) ;
}

static void processRSDT(RSDT *rsdt)
{
    ACPISDTHeader *h = findFACP_in_RSDT(rsdt);
    if (h) {
        struct FADT *fadt_table = (struct FADT *)h;
        processFADT(fadt_table);
    }
}

static void processXSDT(XSDT *xsdt)
{
    ACPISDTHeader *h = findFACP_in_XSDT(xsdt);
    if (h) {
        struct FADT *fadt_table = (struct FADT *)h;
        processFADT(fadt_table);
    }
}

void acpi_init() {
    rsdp_response = get_rsdp();
    if (rsdp_response == NULL) {
        printf("[ ACPI ] RSDP not found.\n");
        hcf();
    }

    ensure_mapped_phys_range((uintptr_t)rsdp_response->address, sizeof(XSDP_t));
    RSDP_t *rsdp = (RSDP_t *)virt_addr((uintptr_t)rsdp_response->address);
    if (rsdp->revision >= 2) {
        XSDP_t *xsdp = (XSDP_t *)virt_addr((uintptr_t)rsdp_response->address);
        ensure_mapped_phys_range((uintptr_t)xsdp->xsdt_address, sizeof(ACPISDTHeader));
        ACPISDTHeader *xh = (ACPISDTHeader *)virt_addr((uintptr_t)xsdp->xsdt_address);
        ensure_mapped_phys_range((uintptr_t)xsdp->xsdt_address, xh->Length);
        processXSDT((XSDT *)virt_addr((uintptr_t)xsdp->xsdt_address));
    } else {
        ensure_mapped_phys_range((uintptr_t)rsdp->rsdt_address, sizeof(ACPISDTHeader));
        ACPISDTHeader *rh = (ACPISDTHeader *)virt_addr((uintptr_t)rsdp->rsdt_address);
        ensure_mapped_phys_range((uintptr_t)rsdp->rsdt_address, rh->Length);
        processRSDT((RSDT *)virt_addr((uintptr_t)rsdp->rsdt_address));
    }

    uacpi_initialize(0);
}

static void keyboard_controller_reboot(void) {
    IoWrite8(0x64, 0xFE);
}

static void qemu_reboot(void) {
    IoWrite16(0xCF9, 0x06);
    IoWrite16(0x604, 0x2000);
    IoWrite8(0xB004, 0x00);
}

static void triple_fault_reboot(void) {
    printf("[ ACPI ] Trying triple fault...\n");
    
    __asm__ volatile("cli");
    
    struct {
        uint16_t limit;
        uint64_t base;
    } __attribute__((packed)) null_idt = {0, 0};
    
    __asm__ volatile("lidt %0" : : "m"(null_idt));
    __asm__ volatile("int $0x03");
}

void acpi_reboot() {
    if (rsdp_response->revision < 2) {
        IoWrite8(0xCF9, 0x06);
        for (volatile int i = 0; i < 100000000; i++);
    }
    
    if (fadt != NULL) {
        size_t reset_offset = offsetof(struct FADT, ResetReg);
        if (fadt->h.Length > reset_offset + sizeof(GenericAddressStructure) && 
            fadt->ResetReg.Address != 0) {
            
            uint8_t addr_space = fadt->ResetReg.AddressSpace;
            
            if (addr_space <= 2) {
                switch (addr_space) {
                    case 0: // System Memory
                        if (fadt->ResetReg.Address < 0x100000000ULL) {
                            ensure_mapped_phys_range((uintptr_t)fadt->ResetReg.Address, 1);
                            *(volatile uint8_t *)virt_addr((uintptr_t)fadt->ResetReg.Address) = fadt->ResetValue;
                        }
                        break;
                    
                    case 1: // System I/O
                        if (fadt->ResetReg.Address < 0x10000) {
                            IoWrite8((uint16_t)fadt->ResetReg.Address, fadt->ResetValue);
                        }
                        break;
                    
                    case 2: // PCI Configuration Space
                        printf("[ ACPI ] PCI config space reset not implemented.\n");
                        break;
                }
                
                for (volatile int i = 0; i < 10000000; i++);
            }
        } else {
            printf("[ ACPI ] FADT too old (Length=%u), no reset register support.\n", fadt->h.Length);
        }
    }
    
    printf("[ ACPI ] Trying alternative reset methods...\n");
    qemu_reboot();
    triple_fault_reboot();
    
    printf("[ ACPI ] All reboot methods failed. Hanging...\n");
    for (;;) {
        __asm__ volatile("pause");
    }
}

static uint8_t get_S5_sleep_type() {
    uacpi_object *s5_pkg = NULL;
    uacpi_status st = uacpi_eval_simple(NULL, "_S5", &s5_pkg);
    if (st != UACPI_STATUS_OK || !s5_pkg) {
        if (s5_pkg) uacpi_object_unref(s5_pkg);
        return 5;
    }
    if (uacpi_object_get_type(s5_pkg) != UACPI_OBJECT_PACKAGE) {
        uacpi_object_unref(s5_pkg);
        return 5;
    }
    uacpi_object_array arr;
    if (uacpi_object_get_package(s5_pkg, &arr) != UACPI_STATUS_OK || arr.count < 2) {
        uacpi_object_unref(s5_pkg);
        return 5;
    }
    uacpi_object *slp_typa = arr.objects[0];
    uint8_t val = 5;
    if (slp_typa && uacpi_object_get_type(slp_typa) == UACPI_OBJECT_INTEGER) {
        uacpi_u64 val64 = 5;
        if (uacpi_object_get_integer(slp_typa, &val64) == UACPI_STATUS_OK)
            val = (uint8_t)val64;
    }
    uacpi_object_unref(s5_pkg);
    return val;
}

void acpi_shutdown() {
    if (!fadt) {
        printf("[ ACPI ] FADT not initialized, cannot shutdown.\n");
        return;
    }

    uint16_t SLP_TYP = get_S5_sleep_type();
    uint16_t SLP_EN = 1 << 13;
    uint16_t pm1a_cnt = fadt->PM1aControlBlock;
    uint16_t pm1b_cnt = fadt->PM1bControlBlock;

    uacpi_object *arg5 = uacpi_object_create_integer(5);
    uacpi_object *ret = NULL;
    uacpi_status st = UACPI_STATUS_INTERNAL_ERROR;
    if (arg5) {
        uacpi_object_array args5 = { .count = 1, .objects = &arg5 };
        st = uacpi_eval(NULL, "_PTS", &args5, &ret);
        if (st == UACPI_STATUS_OK) {
            if (ret) uacpi_object_unref(ret);
        } else {
            printf("[ ACPI ] _PTS(5) not present or failed.\n");
        }
        uacpi_object_unref(arg5);
    } else {
        printf("[ ACPI ] Failed to allocate _PTS argument.\n");
    }

    st = uacpi_eval_simple(NULL, "_GTS", &ret);
    if (st == UACPI_STATUS_OK) {
        if (ret) uacpi_object_unref(ret);
    } else {
        printf("[ ACPI ] _GTS not present or failed.\n");
    }

    uacpi_object *arg0 = uacpi_object_create_integer(0);
    if (arg0) {
        uacpi_object_array args0 = { .count = 1, .objects = &arg0 };
        st = uacpi_eval(NULL, "_SST", &args0, &ret);
        if (st == UACPI_STATUS_OK) {
            if (ret) uacpi_object_unref(ret);
        } else {
            printf("[ ACPI ] _SST(0) not present or failed.\n");
        }
        uacpi_object_unref(arg0);
    } else {
        printf("[ ACPI ] Failed to allocate _SST argument.\n");
    }

    __asm__ volatile ("cli");

    uint16_t sci_val = IoRead16(pm1a_cnt);
    sci_val &= ~1;
    IoWrite16(pm1a_cnt, sci_val);

    uint16_t val = IoRead16(pm1a_cnt);
    val &= 0xE3FF;
    val |= (SLP_TYP << 10) | SLP_EN;
    IoWrite16(pm1a_cnt, val);

    if (pm1b_cnt) {
        uint16_t val2 = IoRead16(pm1b_cnt);
        val2 &= 0xE3FF;
        val2 |= (SLP_TYP << 10) | SLP_EN;
        IoWrite16(pm1b_cnt, val2);
    }

    for (volatile int i = 0; i < 100000000; i++);

    printf("[ ACPI ] ACPI shutdown did not work, trying QEMU port...\n");
    IoWrite16(0x604, 0x2000);
    IoWrite8(0xB004, 0x00);
    for (volatile int i = 0; i < 100000000; i++);

    printf("[ ACPI ] All shutdown methods failed. Halting.\n");
    for (;;) {
        __asm__ volatile ("hlt");
    }
}