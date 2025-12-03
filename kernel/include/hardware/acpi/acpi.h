#ifndef ACPI_H
#define ACPI_H

#include <stdint.h>
#include <stddef.h>

#include <hardware/memory/paging.h>

#include <system/multitasking/spinlock.h>

typedef struct  
{
    char signature[8];
    uint8_t checksum;
    char OEM_id[6];
    uint8_t revision;
    uint32_t rsdt_address;
} __attribute__ ((packed)) RSDP_t;

typedef struct
{
    char signature[8];
    uint8_t checksum;
    char OEMID[6];
    uint8_t revision;
    uint32_t rsdt_address;

    // 2.0+ fields
    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} __attribute__ ((packed)) XSDP_t;


typedef struct ACPISDTHeader {
  char Signature[4];
  uint32_t Length;
  uint8_t Revision;
  uint8_t Checksum;
  char OEMID[6];
  char OEMTableID[8];
  uint32_t OEMRevision;
  uint32_t CreatorID;
  uint32_t CreatorRevision;
} __attribute__ ((packed)) ACPISDTHeader;

typedef struct RSDT {
  ACPISDTHeader h;
  uint32_t PointerToOtherSDT[];
} __attribute__ ((packed)) RSDT;

typedef struct XSDT {
  ACPISDTHeader h;
  uint64_t PointerToOtherSDT[];
} __attribute__ ((packed)) XSDT;

typedef struct GenericAddressStructure
{
  uint8_t AddressSpace;
  uint8_t BitWidth;
  uint8_t BitOffset;
  uint8_t AccessSize;
  uint64_t Address;
} __attribute__ ((packed)) GenericAddressStructure;

void acpi_init();
void acpi_reboot();
void acpi_shutdown();

static inline void ensure_mapped_phys_range(uintptr_t phys, size_t size)
{
  // Skip kernel higher-half addresses (already mapped)
  if (phys >= 0xffffffff80000000ULL) {
    return;
  }
  
  // Limine's HHDM should map all physical memory, but some regions
  // (like ACPI reclaimable) may not be properly mapped in UEFI mode.
  // We need to manually map these pages.
  extern uint64_t hhdm;
  
  uintptr_t page_start = phys & ~0xFFFULL;
  uintptr_t page_end = (phys + size + 0xFFF) & ~0xFFFULL;
  
  for (uintptr_t p = page_start; p < page_end; p += 0x1000) {
    uintptr_t virt = hhdm + p;
    // Map with PG_PRESENT | PG_WRITABLE
    mapPage((void*)virt, (void*)p, PG_PRESENT | PG_WRITABLE);
  }
}

#endif