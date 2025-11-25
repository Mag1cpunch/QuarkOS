#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <stdbool.h>

#include <kernel.h>

/* PTE/PDE/PDPTE/PML4E flags (Intel SDM §4.5) */
#define PG_PRESENT        (1ULL << 0)
#define PG_WRITABLE       (1ULL << 1)
#define PG_USER           (1ULL << 2)
#define PG_PWT            (1ULL << 3)
#define PG_PCD            (1ULL << 4)
#define PG_ACCESSED       (1ULL << 5)
#define PG_DIRTY          (1ULL << 6)
#define PG_PAT            (1ULL << 7)   /* only PT level */
#define PG_GLOBAL         (1ULL << 8)   /* only PT level */
#define PG_NX             (1ULL << 63)  /* if EFER.NXE */

#define PAGE_SIZE 0x1000

typedef struct PageEntry {
    uint8_t present : 1;
    uint8_t writable : 1;
    uint8_t user_accessible : 1;
    uint8_t write_through_caching : 1;
    uint8_t disable_cache : 1;
    uint8_t accessed : 1;
    uint8_t dirty : 1;
    uint8_t null : 1;
    uint8_t global : 1;
    uint8_t avl1 : 3;
    uintptr_t physical_address : 40;
    uint16_t avl2 : 11;
    uint8_t no_execute : 1;
} PageEntry;

typedef struct PageTable {
    PageEntry entries[512];
} PageTable;

typedef struct page_meta {
    uint16_t live;   /* number of allocated blocks */
    uint16_t flags;  /* optional */
} page_meta_t;

extern uint64_t hhdm;

extern PageTable* pml4;
extern uint64_t kernel_cr3_phys;

void* getPhysicalAddress(void* virtual_address); 
PageTable* initPML4(void); 
void mapPage(void* virtual_address, void* physical_address, uint64_t flags);
void mapPage_in_pml4(uint64_t pml4_phys, void *virt, void *phys, uint64_t flags);
uint64_t readCR3(void);
uintptr_t page_base(void *p);
void unmapPage(void *virtual_address);
void page_inc_live(void *block);
bool page_dec_live_check_empty(void *block);
uint64_t create_user_address_space(void);
uint64_t virt_to_phys_in_pml4(uint64_t pml4_phys, void *virt);

static inline void* virt_addr(uintptr_t phys) {
    return (void*)(phys + hhdm);
}

static inline uintptr_t phys_addr(void* virt) {
    return (uintptr_t)virt - hhdm;
}
#endif
