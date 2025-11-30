#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <lai/host.h>

#include <arch/x86_64/apic/apic.h>

#include <hardware/acpi/acpi.h>
#include <hardware/acpi/descriptor_tables/fadt.h>
#include <hardware/memory/heap.h>
#include <hardware/memory/paging.h>
#include <hardware/devices/io.h>
#include <hardware/requests.h>

void laihost_log(int level, const char *msg) {
    const char *level_str = "";
    switch (level) {
        case LAI_DEBUG_LOG:
            level_str = "DEBUG";
            break;
        case LAI_WARN_LOG:
            level_str = "WARN";
            break;
        default:
            level_str = "LOG";
            break;
    }
    printf("[ LAI %s ] %s\n", level_str, msg);
}

__attribute__((noreturn)) void laihost_panic(const char *msg) {
    printf("[ LAI PANIC ] %s\n", msg);
    for (;;) { __asm__("cli; hlt"); }
}

void* laihost_malloc(size_t size) {
    return kmalloc(size);
}

void* laihost_realloc(void *old, size_t newsize, size_t oldsize) {
    return krealloc(old, newsize);
}

void laihost_free(void *ptr, size_t size) {
    kfree(ptr);
}

__attribute__((weak)) void *laihost_map(size_t address, size_t count) {
    uintptr_t phys = (uintptr_t)address;
    ensure_mapped_phys_range(phys, count);
    void *virt = (void *)virt_addr(phys);
    return virt;
}

__attribute__((weak)) void laihost_unmap(void *addr, size_t count) {
    // No action needed
}

__attribute__((weak)) void *laihost_scan(const char *sig, size_t index) {
    struct limine_rsdp_response *rsdp_response = get_rsdp();
    if (!rsdp_response || !rsdp_response->address)
        return NULL;

    if (strcmp(sig, "RSD PTR ") == 0) {
        if (index == 0)
            return (void *)rsdp_response->address;
        else
            return NULL;
    }

    if (!strncmp(sig, "DSDT", 4) && index == 0) {
        if (fadt && fadt->dsdt) {
            ensure_mapped_phys_range((uintptr_t)fadt->dsdt, sizeof(ACPISDTHeader));
            ACPISDTHeader *dsdt_h = (ACPISDTHeader *)virt_addr((uintptr_t)fadt->dsdt);
            ensure_mapped_phys_range((uintptr_t)fadt->dsdt, dsdt_h->Length);
            return (void *)dsdt_h;
        } else {
            printf("[LAIHOST_SCAN] No FADT or DSDT pointer for DSDT scan!\n");
            return NULL;
        }
    }

    uintptr_t rsdp_phys = (uintptr_t)rsdp_response->address;
    XSDP_t *xsdp = (XSDP_t *)virt_addr(rsdp_phys);
    int found = 0;

    if (xsdp->revision >= 2 && xsdp->xsdt_address && xsdp->xsdt_address <= 0xFFFFFFFFFFFF) {
        uintptr_t xsdt_phys = (uintptr_t)xsdp->xsdt_address;
        struct XSDT *xsdt = (struct XSDT *)virt_addr(xsdt_phys);
        int entries = (xsdt->h.Length - sizeof(xsdt->h)) / 8;
        for (int i = 0; i < entries; i++) {
            uint64_t entry_phys;
            memcpy(&entry_phys, (uint8_t*)xsdt->PointerToOtherSDT + i * 8, 8);
            ACPISDTHeader *h = (ACPISDTHeader *)virt_addr((uintptr_t)entry_phys);
            if (!strncmp(h->Signature, sig, 4)) {
                if (found == index) {
                    ensure_mapped_phys_range((uintptr_t)entry_phys, h->Length);
                    void *ret = (void *)virt_addr((uintptr_t)entry_phys);
                    return ret;
                }
                found++;
            }
        }
    }

    uintptr_t rsdt_phys = (uintptr_t)xsdp->rsdt_address;
    if (rsdt_phys && rsdt_phys <= 0xFFFFFFFF) {
        struct RSDT *rsdt = (struct RSDT *)virt_addr(rsdt_phys);
        int entries = (rsdt->h.Length - sizeof(rsdt->h)) / 4;
        for (int i = 0; i < entries; i++) {
            uint32_t entry_phys;
            memcpy(&entry_phys, (uint8_t*)rsdt->PointerToOtherSDT + i * 4, 4);
            ACPISDTHeader *h = (ACPISDTHeader *)virt_addr((uintptr_t)entry_phys);
            if (!strncmp(h->Signature, sig, 4)) {
                if (found == index) {
                    ensure_mapped_phys_range((uintptr_t)entry_phys, h->Length);
                    void *ret = (void *)virt_addr((uintptr_t)entry_phys);
                    return ret;
                }
                found++;
            }
        }
    }

    return NULL;
}

__attribute__((weak)) void laihost_outb(uint16_t port, uint8_t val) {
    IoWrite8(port, val);
}

__attribute__((weak)) void laihost_outw(uint16_t port, uint16_t val) {
    IoWrite16(port, val);
}

__attribute__((weak)) void laihost_outd(uint16_t port, uint32_t val) {
    IoWrite32(port, val);
}

__attribute__((weak)) uint8_t laihost_inb(uint16_t port) {
    return IoRead8(port);
}

__attribute__((weak)) uint16_t laihost_inw(uint16_t port) {
    return IoRead16(port);
}

__attribute__((weak)) uint32_t laihost_ind(uint16_t port) {
    return IoRead32(port);
}

__attribute__((weak)) void laihost_sleep(uint64_t ms) {
    apic_timer_sleep_ms(ms);
}

static uint32_t pci_config_address(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t func, uint16_t offset) {
    (void)seg;
    return (1U << 31) | ((uint32_t)bus << 16) | ((uint32_t)slot << 11) | ((uint32_t)func << 8) | (offset & 0xFC);
}

__attribute__((weak)) uint8_t laihost_pci_readb(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t func, uint16_t offset) {
    uint32_t addr = pci_config_address(seg, bus, slot, func, offset);
    IoWrite32(0xCF8, addr);
    uint32_t val = IoRead32(0xCFC);
    uint8_t shift = (offset & 3) * 8;
    uint8_t result = (val >> shift) & 0xFF;
    return result;
}

__attribute__((weak)) uint16_t laihost_pci_readw(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t func, uint16_t offset) {
    uint32_t addr = pci_config_address(seg, bus, slot, func, offset);
    IoWrite32(0xCF8, addr);
    uint32_t val = IoRead32(0xCFC);
    uint8_t shift = (offset & 2) * 8;
    uint16_t result = (val >> shift) & 0xFFFF;
    return result;
}

__attribute__((weak)) uint32_t laihost_pci_readd(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t func, uint16_t offset) {
    uint32_t addr = pci_config_address(seg, bus, slot, func, offset);
    IoWrite32(0xCF8, addr);
    uint32_t result = IoRead32(0xCFC);
    return result;
}