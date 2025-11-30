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

#include <hardware/memory/paging.h>
#include <stdint.h>
#include <stddef.h>

static inline void ensure_mapped_phys_range(uintptr_t phys, size_t size)
{
  uintptr_t start = (phys & ~(PAGE_SIZE - 1)) - 3 * PAGE_SIZE; // map three pages before
  uintptr_t end   = (phys + size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
  extern uint64_t hhdm;
  for (uintptr_t p = start; p < end; p += PAGE_SIZE) {
    void *v = (void *)virt_addr(p);
    mapPage(v, (void *)p, PG_WRITABLE | PG_NX);
  }
}

#endif