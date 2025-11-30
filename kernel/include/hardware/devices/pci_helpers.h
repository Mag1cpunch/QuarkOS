#ifdef PCIHELPERS_H
#error this file should only be included ONCE and only in the io/pci.c file
#else
#define PCIHELPERS_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <arch/x86_64/cpu.h>

#include <hardware/devices/io.h>
#include <hardware/memory/pmm.h>
#include <hardware/memory/paging.h>
#include <hardware/memory/mmio.h>
#include <hardware/memory/heap.h>
#include <hardware/acpi/uacpi/uacpi.h>
#include <hardware/acpi/uacpi/tables.h>
#include <hardware/acpi/uacpi/internal/tables.h>

extern size_t mcfgentrycount;
extern struct acpi_mcfg_allocation *mcfgentries;
extern uint32_t (*pci_archread32)(int bus, int device, int function, uint32_t offset);
extern void (*pci_archwrite32)(int bus, int device, int function, uint32_t offset, uint32_t value);

#define CONFADD 0xcf8
#define CONFDATA 0xcfc

static uint32_t legacy_read32(int bus, int device, int function, uint32_t offset) {
	uint32_t confadd = 0x80000000 | (offset & ~0x3) | (function << 8) | (device << 11) | (bus << 16);
    IoWrite32(CONFADD, confadd);
	return IoRead32(CONFDATA);
}

static void legacy_write32(int bus, int device, int function, uint32_t offset, uint32_t value) {
	uint32_t confadd = 0x80000000 | (offset & ~0x3) | (function << 8) | (device << 11) | (bus << 16);
	IoWrite32(CONFADD, confadd);
	IoWrite32(CONFDATA, value);
}

static inline struct acpi_mcfg_allocation *getmcfgentry(int bus) {
	struct acpi_mcfg_allocation *entry;
	int i;

	for (i = 0; i < mcfgentrycount; ++i) {
		entry = &mcfgentries[i];
		if (bus >= entry->start_bus && bus <= entry->end_bus)
			break;
	}

	if (i == mcfgentrycount)
		entry = NULL;

	return entry;
}

static uint32_t mcfg_read32(int bus, int device, int function, uint32_t offset) {
	struct acpi_mcfg_allocation *entry = getmcfgentry(bus);
	if (entry == NULL)
		return 0xffffffff;

	uint64_t base = (uint64_t)entry->address;
	volatile uint32_t *address = (uint32_t *)((uintptr_t)base + (((bus - entry->start_bus) << 20) | (device << 15) | (function << 12) | (offset & ~0x3)));

	return *address;
}

static void mcfg_write32(int bus, int device, int function, uint32_t offset, uint32_t value) {
	struct acpi_mcfg_allocation *entry = getmcfgentry(bus);

	uint64_t base = (uint64_t)entry->address;
	volatile uint32_t *address = (uint32_t *)((uintptr_t)base + (((bus - entry->start_bus) << 20) | (device << 15) | (function << 12) | (offset & ~0x3)));

	*address = value;
}

static uint64_t msiformatmessage(uint32_t *data, int vector, int edgetrigger, int deassert) {
	*data = (edgetrigger ? 0 : (1 << 15)) | (deassert ? 0 : (1 << 14)) | vector;
	return 0xfee00000 | (current_cpu_id() << 12);
}

// in PCIe, every function has 4096 bytes of config space for it.
// there are 8 functions per device and 32 devices per bus
#define MCFG_MAPPING_SIZE(buscount) (4096l * 8l * 32l * (buscount))

void pci_archinit();

#endif // PCIHELPERS_H