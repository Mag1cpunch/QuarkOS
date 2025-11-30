#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <limine.h>

#include <arch/x86_64/cpu.h>

#include <lai/core.h>
#include <lai/helpers/pm.h>
#include <lai/helpers/sci.h>

#include <hardware/acpi/acpi.h>
#include <hardware/acpi/descriptor_tables/fadt.h>
#include <hardware/requests.h>
#include <hardware/memory/paging.h>
#include <hardware/devices/io.h>

#include <system/multitasking/spinlock.h>
#include <system/multitasking/tasksched.h>

static struct limine_rsdp_response *rsdp_response;
struct FADT *fadt;

static ACPISDTHeader *findFACP_in_XSDT(XSDT *xsdt)
{
    int entries = (xsdt->h.Length - sizeof(xsdt->h)) / 8;
    for (int i = 0; i < entries; i++) {
        uint64_t raw;
        memcpy(&raw, (uint8_t*)xsdt->PointerToOtherSDT + i * 8, 8);
        uintptr_t phys = (uintptr_t)raw;
        ensure_mapped_phys_range(phys, sizeof(ACPISDTHeader));
        ACPISDTHeader *h = (ACPISDTHeader *)(uintptr_t)virt_addr(phys);
        if (!strncmp(h->Signature, "FACP", 4)) {
            
            uint8_t sum = 0;
            for (unsigned i = 0; i < h->Length; ++i) sum += ((uint8_t*)h)[i];
            return h;
        }
    }
    return NULL;
}

static ACPISDTHeader *findFACP_in_RSDT(RSDT *rsdt)
{
    int entries = (rsdt->h.Length - sizeof(rsdt->h)) / 4;
    for (int i = 0; i < entries; i++) {
        uint32_t raw;
        memcpy(&raw, (uint8_t*)rsdt->PointerToOtherSDT + i * 4, 4);
        uintptr_t phys = (uintptr_t)raw;
        ensure_mapped_phys_range(phys, 64);
        uint8_t *dump = (uint8_t *)virt_addr(phys);
        for (int j = 0; j < 64; j++) {
            printf("%02x ", dump[j]);
            if ((j+1) % 16 == 0) printf("\n");
        }
        ACPISDTHeader *h = (ACPISDTHeader *)dump;
        if (!strncmp(h->Signature, "FACP", 4)) {
            uint8_t sum = 0;
            for (unsigned i = 0; i < h->Length; ++i) sum += ((uint8_t*)h)[i];
            return h;
        }
    }
    return NULL;
}

static inline void processFADT(struct FADT *fadt_table)
{
    uintptr_t fadt_phys = phys_addr(fadt_table);
    size_t map_size = fadt_table->h.Length > sizeof(struct FADT) ? fadt_table->h.Length : sizeof(struct FADT);
    ensure_mapped_phys_range(fadt_phys, map_size);

    if (fadt_table->h.Revision >= 2) {
        fadt_table->x_dsdt = 0;
    }
    if (fadt_table->dsdt) {
        ensure_mapped_phys_range((uintptr_t)fadt_table->dsdt, sizeof(ACPISDTHeader));
        ACPISDTHeader *dsdt_h = (ACPISDTHeader *)virt_addr((uintptr_t)fadt_table->dsdt);
        ensure_mapped_phys_range((uintptr_t)fadt_table->dsdt, dsdt_h->Length);
    }
    // ACPI 2.0+ X_Dsdt field (64-bit)
    if (fadt_table->h.Revision >= 2 && fadt_table->x_dsdt) {
        ensure_mapped_phys_range((uintptr_t)fadt_table->x_dsdt, sizeof(ACPISDTHeader));
        ACPISDTHeader *xdsdt_h = (ACPISDTHeader *)virt_addr((uintptr_t)fadt_table->x_dsdt);
        ensure_mapped_phys_range((uintptr_t)fadt_table->x_dsdt, xdsdt_h->Length);
    }

    fadt = fadt_table;

    if (fadt->acpi_disable == 0 && fadt->acpi_enable == 0 && fadt->smi_command_port == 0) {
        printf("[ ACPI ] ACPI already enabled. Skipping...\n");
        return;
    }

    IoWrite8(fadt->smi_command_port, fadt->acpi_enable);

    while (IoRead16(fadt->pm1a_control_block) & 1 == 0) ;
}

static void processRSDT(RSDT *rsdt)
{
    // HACK: Force-align all SDT pointers in the RSDT to 4 bytes
    int entries = (rsdt->h.Length - sizeof(rsdt->h)) / 4;
    for (int i = 0; i < entries; i++) {
        uint32_t *ptr = (uint32_t *)((uint8_t*)rsdt->PointerToOtherSDT + i * 4);
        if ((*ptr & 0x3) != 0) {
            *ptr &= ~0x3;
        }
    }
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
    if (!rsdp_response || !rsdp_response->address) {
        printf("[ ACPI ] No RSDP found!\n");
        return;
    }

    if (!rsdp_response) { printf("[ ACPI DEBUG ] rsdp_response is NULL!\n"); hcf(); }
    uintptr_t acpi_min = (uintptr_t)-1;
    uintptr_t acpi_max = 0;
    
    ensure_mapped_phys_range((uintptr_t)rsdp_response->address, sizeof(XSDP_t));
    XSDP_t *xsdp_map = (XSDP_t *)virt_addr((uintptr_t)rsdp_response->address);
    if (!xsdp_map) { printf("[ ACPI DEBUG ] xsdp_map is NULL!\n"); hcf(); }
    int use_xsdt = (xsdp_map->revision >= 2 && xsdp_map->xsdt_address != 0 && xsdp_map->xsdt_address <= 0xFFFFFFFFFFFF);
    if (use_xsdt) {
        XSDT *xsdt = (XSDT *)virt_addr((uintptr_t)xsdp_map->xsdt_address);
        if (!xsdt) { printf("[ ACPI DEBUG ] xsdt is NULL!\n"); hcf(); }
        int entries = (xsdt->h.Length - sizeof(xsdt->h)) / 8;
        for (int i = 0; i < entries; i++) {
            uint64_t entry_phys;
            memcpy(&entry_phys, (uint8_t*)xsdt->PointerToOtherSDT + i * 8, 8);
            ensure_mapped_phys_range((uintptr_t)entry_phys, sizeof(ACPISDTHeader));
            ACPISDTHeader *h = (ACPISDTHeader *)virt_addr((uintptr_t)entry_phys);
            if (!h) { printf("[ ACPI DEBUG ] XSDT entry %d header is NULL!\n", i); hcf(); }
            uintptr_t start = (uintptr_t)entry_phys;
            uintptr_t end = start + h->Length;
            if (start < acpi_min) acpi_min = start;
            if (end > acpi_max) acpi_max = end;
        }
        uintptr_t xsdt_start = (uintptr_t)xsdp_map->xsdt_address;
        uintptr_t xsdt_end = xsdt_start + xsdt->h.Length;
        if (xsdt_start < acpi_min) acpi_min = xsdt_start;
        if (xsdt_end > acpi_max) acpi_max = xsdt_end;
    } else {
        RSDT *rsdt = (RSDT *)virt_addr((uintptr_t)xsdp_map->rsdt_address);
        if (!rsdt) { printf("[ ACPI DEBUG ] rsdt is NULL!\n"); hcf(); }
        int entries = (rsdt->h.Length - sizeof(rsdt->h)) / 4;
        for (int i = 0; i < entries; i++) {
            uint32_t entry_phys;
            memcpy(&entry_phys, (uint8_t*)rsdt->PointerToOtherSDT + i * 4, 4);
            ACPISDTHeader *h = (ACPISDTHeader *)virt_addr((uintptr_t)entry_phys);
            if (!h) { printf("[ ACPI DEBUG ] RSDT entry %d header is NULL!\n", i); hcf(); }
            uintptr_t start = (uintptr_t)entry_phys;
            uintptr_t end = start + h->Length;
            if (start < acpi_min) acpi_min = start;
            if (end > acpi_max) acpi_max = end;
        }
        uintptr_t rsdt_start = (uintptr_t)xsdp_map->rsdt_address;
        uintptr_t rsdt_end = rsdt_start + rsdt->h.Length;
        if (rsdt_start < acpi_min) acpi_min = rsdt_start;
        if (rsdt_end > acpi_max) acpi_max = rsdt_end;
    }
    uintptr_t rsdp_start = (uintptr_t)rsdp_response->address;
    uintptr_t rsdp_end = rsdp_start + sizeof(XSDP_t);
    if (rsdp_start < acpi_min) acpi_min = rsdp_start;
    if (rsdp_end > acpi_max) acpi_max = rsdp_end;
    
    acpi_min &= ~(PAGE_SIZE - 1);
    acpi_max = (acpi_max + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    for (uintptr_t p = acpi_min; p < acpi_max; p += PAGE_SIZE) {
        void *v = (void *)virt_addr(p);
        mapPage(v, (void *)p, PG_WRITABLE | PG_NX);
    }

    rsdp_response = get_rsdp();
    if (rsdp_response == NULL) {
        printf("[ ACPI ] RSDP not found.\n");
        hcf();
    }

    if (rsdp_response->address == 0 || rsdp_response->address > 0xFFFFFFFFFFFF) {
        printf("[ ACPI ] Invalid RSDP address!\n");
        hcf();
    }

    ensure_mapped_phys_range((uintptr_t)rsdp_response->address, sizeof(XSDP_t));
    XSDP_t *xsdp = (XSDP_t *)virt_addr((uintptr_t)rsdp_response->address);

    if (xsdp->revision >= 2 && xsdp->xsdt_address != 0 && xsdp->xsdt_address <= 0xFFFFFFFFFFFF) {
        ensure_mapped_phys_range((uintptr_t)xsdp->xsdt_address, sizeof(ACPISDTHeader));
        ACPISDTHeader *xh = (ACPISDTHeader *)virt_addr((uintptr_t)xsdp->xsdt_address);
        if (xh->Length < sizeof(ACPISDTHeader) || xh->Length > 0x10000) {
            printf("[ ACPI ] XSDT header length invalid!\n");
            hcf();
        }
        ensure_mapped_phys_range((uintptr_t)xsdp->xsdt_address, xh->Length);
        processXSDT((XSDT *)virt_addr((uintptr_t)xsdp->xsdt_address));
    } else {
        if (xsdp->rsdt_address == 0 || xsdp->rsdt_address > 0xFFFFFFFF) {
            printf("[ ACPI ] Invalid RSDT address!\n");
            hcf();
        }
        ensure_mapped_phys_range((uintptr_t)xsdp->rsdt_address, sizeof(ACPISDTHeader));
        ACPISDTHeader *rh = (ACPISDTHeader *)virt_addr((uintptr_t)xsdp->rsdt_address);
        if (rh->Length < sizeof(ACPISDTHeader) || rh->Length > 0x10000) {
            printf("[ ACPI ] RSDT header length invalid!\n");
            hcf();
        }
        ensure_mapped_phys_range((uintptr_t)xsdp->rsdt_address, rh->Length);
        processRSDT((RSDT *)virt_addr((uintptr_t)xsdp->rsdt_address));
    }

    if (fadt) {
        uintptr_t fadt_phys = phys_addr(fadt);
        if (fadt->h.Revision >= 2 && (fadt->x_dsdt == 0x200100000000ULL || fadt->x_dsdt > 0xFFFFFFFFFFFFULL || fadt->x_dsdt < 0x1000)) {
            fadt->x_dsdt = 0;
        }
    }
    lai_set_acpi_revision(rsdp_response->revision);
    lai_create_namespace();

    extern uint8_t apic_init_done;
    if (apic_init_done) {
        lai_enable_acpi(1);
    }
    else {
        lai_enable_acpi(0);
    }
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
    lai_acpi_reset();

    printf("[ ACPI ] Trying alternative reset methods...\n");
    qemu_reboot();
    triple_fault_reboot();
    
    printf("[ ACPI ] All reboot methods failed. Hanging...\n");
    for (;;) {
        __asm__ volatile("pause");
    }
}

void acpi_shutdown() {
    lai_enter_sleep(5);

    printf("[ ACPI ] ACPI shutdown did not work, trying QEMU port...\n");
    IoWrite16(0x604, 0x2000);
    IoWrite8(0xB004, 0x00);
    for (volatile int i = 0; i < 100000000; i++);

    printf("[ ACPI ] All shutdown methods failed. Halting.\n");
    for (;;) {
        __asm__ volatile ("hlt");
    }
}