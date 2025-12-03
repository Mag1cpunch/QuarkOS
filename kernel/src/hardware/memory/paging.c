#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include <kernel.h>

#include <hardware/memory/paging.h>
#include <hardware/memory/pmm.h>
#include <hardware/requests.h>

extern struct limine_memmap_entry** memmaps;

PageTable* pml4;
uint64_t kernel_cr3_phys;

static inline void flushTLB(void* page) 
{
	__asm__ volatile ("invlpg (%0)" :: "r" (page) : "memory");
}

uint64_t readCR3(void)
{
    uint64_t val;
    __asm__ volatile ( "mov %%cr3, %0" : "=r"(val) );
    return val;
}

PageTable* initPML4() {
    uintptr_t cr3 = (uintptr_t)readCR3();
    kernel_cr3_phys = cr3 & ~0xFFF;

    uintptr_t pml4_phys = (cr3 >> 12) << 12;
    pml4 = (PageTable *)virt_addr(pml4_phys);
    return pml4;
}

static void setPageTableEntry(PageEntry *e, uint64_t flags, uint64_t phys_page) {
    e->present    = (flags & PG_PRESENT) ? 1 : 0;
    e->writable   = (flags & PG_WRITABLE) ? 1 : 0;
    e->user_accessible = (flags & PG_USER) ? 1 : 0;
    e->no_execute = (flags & PG_NX) ? 1 : 0;
    e->physical_address = phys_page;
}

void* getPhysicalAddress(void* virtual_address) 
{
    uintptr_t virt = (uintptr_t)virtual_address;

    uint64_t pml4_i  = (virt >> 39) & 0x1FF;
    uint64_t pdpt_i  = (virt >> 30) & 0x1FF;
    uint64_t pd_i    = (virt >> 21) & 0x1FF;
    uint64_t pt_i    = (virt >> 12) & 0x1FF;
    uint64_t offset  = virt & 0xFFF;

    PageTable *pml4_tbl = pml4;
    if (!pml4_tbl->entries[pml4_i].present) {
        printf("[ ERROR ] PML4[%llu] not present\n", pml4_i);
        return 0;
    }

    PageTable *pdpt = (PageTable*)virt_addr(
        (uintptr_t)pml4_tbl->entries[pml4_i].physical_address << 12);

    if (!pdpt->entries[pdpt_i].present) {
        printf("[ ERROR ] PDPT[%llu] not present\n", pdpt_i);
        return 0;
    }

    PageTable *pd = (PageTable*)virt_addr(
        (uintptr_t)pdpt->entries[pdpt_i].physical_address << 12);

    if (!pd->entries[pd_i].present) {
        printf("[ ERROR ] PD[%llu] not present\n", pd_i);
        return 0;
    }

    PageTable *pt = (PageTable*)virt_addr(
        (uintptr_t)pd->entries[pd_i].physical_address << 12);

    if (!pt->entries[pt_i].present) {
        printf("[ ERROR ] PT[%llu] not present\n", pt_i);
        return 0;
    }

    uintptr_t phys_base = (uintptr_t)pt->entries[pt_i].physical_address << 12;
    return (void *)(phys_base + offset);
}

static void allocateEntry(PageTable* table, size_t index, uint64_t flags)
{
    uint64_t phys = alloc_page();
    if (!phys) {
        printf("[ ERROR ] OOM in allocateEntry! (or phys 0 allocated)\n");
        return;
    }

    void *virt = (void *)virt_addr(phys);
    memset(virt, 0, PAGE_SIZE);

    uint64_t entry_flags = PG_PRESENT | PG_WRITABLE;
    if (flags & PG_USER) {
        entry_flags |= PG_USER;
    }

    setPageTableEntry(&table->entries[index], entry_flags, phys >> 12);
}

void mapPage(void* virtual_address, void* physical_address, uint64_t flags) 
{
    uintptr_t virtual_address_int = (uintptr_t) virtual_address;
    uintptr_t physical_address_int = (uintptr_t) physical_address;

    uint64_t pml4_index = (virtual_address_int >> 39) & 0x1FF;
    uint64_t page_directory_pointer_index = (virtual_address_int >> 30) & 0x1FF;
    uint64_t page_directory_index = (virtual_address_int >> 21) & 0x1FF;
    uint64_t page_table_index = (virtual_address_int >> 12) & 0x1FF;

    if (!pml4->entries[pml4_index].present) {
        allocateEntry(pml4, pml4_index, flags);
        if (!pml4->entries[pml4_index].present) {
             return;
        }
    } else {
        if (flags & PG_USER) pml4->entries[pml4_index].user_accessible = 1;
        pml4->entries[pml4_index].writable = 1;
    }

    uintptr_t pdpt_phys = (uintptr_t)pml4->entries[pml4_index].physical_address << 12;
    PageTable* page_directory_pointer = (PageTable*) virt_addr(pdpt_phys);

    if (!page_directory_pointer->entries[page_directory_pointer_index].present) {
        allocateEntry(page_directory_pointer, page_directory_pointer_index, flags);
        if (!page_directory_pointer->entries[page_directory_pointer_index].present) {
             return;
        }
    } else {
        if (flags & PG_USER) page_directory_pointer->entries[page_directory_pointer_index].user_accessible = 1;
        page_directory_pointer->entries[page_directory_pointer_index].writable = 1;
    }

    PageTable* page_directory = (PageTable*) virt_addr((uintptr_t)(page_directory_pointer->entries[page_directory_pointer_index].physical_address << 12));

    if (!page_directory->entries[page_directory_index].present) {
        allocateEntry(page_directory, page_directory_index, flags);
        if (!page_directory->entries[page_directory_index].present) {
             printf("[ ERROR ] Failed to allocate PD entry %llu\n", page_directory_index);
             return;
        }
    } else {
        if (flags & PG_USER) page_directory->entries[page_directory_index].user_accessible = 1;
        page_directory->entries[page_directory_index].writable = 1;
    }

    PageTable* page_table = (PageTable*) virt_addr((uintptr_t)(page_directory->entries[page_directory_index].physical_address << 12));

    setPageTableEntry(&page_table->entries[page_table_index],
                      flags | PG_PRESENT,
                      physical_address_int >> 12);

    flushTLB(virtual_address);
}

void map_region(void *virt, void *phys, size_t size, uint64_t flags) {
    uintptr_t v = (uintptr_t)virt;
    uintptr_t p = (uintptr_t)phys;
    size_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    for (size_t i = 0; i < pages; ++i) {
        mapPage((void *)(v + i * PAGE_SIZE), (void *)(p + i * PAGE_SIZE), flags);
    }
}

uint64_t create_user_address_space(void) {
    uint64_t new_phys = alloc_page();
    PageTable *new_pml4 = (PageTable *)virt_addr(new_phys);
    memset(new_pml4, 0, PAGE_SIZE);

    PageTable *kernel_pml4 = pml4;

    for (size_t i = 256; i < 512; ++i)
        new_pml4->entries[i] = kernel_pml4->entries[i];

    return new_phys;
}

uintptr_t page_base(void *p) {
    return ((uintptr_t)p) & ~(PAGE_SIZE - 1);
}

void unmapPage(void *virtual_address) {
    uintptr_t virt = (uintptr_t)virtual_address;
    uint64_t pml4_i  = (virt >> 39) & 0x1FF;
    uint64_t pdpt_i  = (virt >> 30) & 0x1FF;
    uint64_t pd_i    = (virt >> 21) & 0x1FF;
    uint64_t pt_i    = (virt >> 12) & 0x1FF;

    // Presence checks for each level
    if (!pml4->entries[pml4_i].present) return;
    PageTable *pdpt = (PageTable *)virt_addr((uintptr_t)pml4->entries[pml4_i].physical_address << 12);
    if (!pdpt->entries[pdpt_i].present) return;
    PageTable *pd   = (PageTable *)virt_addr((uintptr_t)pdpt->entries[pdpt_i].physical_address << 12);
    if (!pd->entries[pd_i].present) return;
    PageTable *pt   = (PageTable *)virt_addr((uintptr_t)pd->entries[pd_i].physical_address << 12);
    if (!pt->entries[pt_i].present) return;

    pt->entries[pt_i].present = 0;
    flushTLB(virtual_address);
}

void page_inc_live(void *block) {
    uintptr_t base = page_base(block);
    page_meta_t *pm = (page_meta_t *)base;
    pm->live++;
}

bool page_dec_live_check_empty(void *block) {
    uintptr_t base = page_base(block);
    page_meta_t *pm = (page_meta_t *)base;
    if (pm->live > 0) pm->live--;
    return pm->live == 0;
}

static PageTable *pml4_from_phys(uint64_t phys) {
    return (PageTable *)virt_addr(phys);
}

void mapPage_in_pml4(uint64_t pml4_phys, void *virt, void *phys, uint64_t flags) {
    PageTable *save = pml4;
    pml4 = pml4_from_phys(pml4_phys);
    
    //uintptr_t va = (uintptr_t)virt;
    //if (va >= 0x400000 && va < 0x500000) {
        //printf("[MAP] CR3=%#llx VA=%p PA=%p flags=%#llx NX=%d\n", 
        //       pml4_phys, virt, phys, flags, !!(flags & PG_NX));
    //}
    
    mapPage(virt, phys, flags);
    pml4 = save;
}

uint64_t virt_to_phys_in_pml4(uint64_t pml4_phys, void *virt) {
    uintptr_t va = (uintptr_t)virt;
    
    uint64_t pml4_i = (va >> 39) & 0x1FF;
    uint64_t pdpt_i = (va >> 30) & 0x1FF;
    uint64_t pd_i   = (va >> 21) & 0x1FF;
    uint64_t pt_i   = (va >> 12) & 0x1FF;
    uint64_t offset = va & 0xFFF;
    
    PageTable *pml4_table = (PageTable *)virt_addr(pml4_phys);
    if (!pml4_table->entries[pml4_i].present)
        return 0;
    
    PageTable *pdpt = (PageTable *)virt_addr(
        (uintptr_t)pml4_table->entries[pml4_i].physical_address << 12);
    if (!pdpt->entries[pdpt_i].present)
        return 0;
    
    PageTable *pd = (PageTable *)virt_addr(
        (uintptr_t)pdpt->entries[pdpt_i].physical_address << 12);
    if (!pd->entries[pd_i].present)
        return 0;
    
    PageTable *pt = (PageTable *)virt_addr(
        (uintptr_t)pd->entries[pd_i].physical_address << 12);
    if (!pt->entries[pt_i].present)
        return 0;
    
    uint64_t phys_base = (uint64_t)pt->entries[pt_i].physical_address << 12;
    return phys_base + offset;
}

void debug_walk_paging(uint64_t pml4_phys, uint64_t virt) {
    PageTable *pml4 = (PageTable *)virt_addr(pml4_phys);
    
    uint64_t pml4_i = (virt >> 39) & 0x1FF;
    uint64_t pdpt_i = (virt >> 30) & 0x1FF;
    uint64_t pd_i   = (virt >> 21) & 0x1FF;
    uint64_t pt_i   = (virt >> 12) & 0x1FF;
    
    if (!pml4->entries[pml4_i].present) {
        printf("[ ERROR ] PML4 entry not present!\n");
        return;
    }
           
    PageTable *pdpt = (PageTable *)virt_addr((uintptr_t)pml4->entries[pml4_i].physical_address << 12);
    if (!pdpt->entries[pdpt_i].present) {
        printf("[ ERROR ] PDPT entry not present!\n");
        return;
    }

    PageTable *pd = (PageTable *)virt_addr((uintptr_t)pdpt->entries[pdpt_i].physical_address << 12);
    if (!pd->entries[pd_i].present) {
        printf("[ ERROR ] PD entry not present!\n");
        return;
    }

    PageTable *pt = (PageTable *)virt_addr((uintptr_t)pd->entries[pd_i].physical_address << 12);
    if (!pt->entries[pt_i].present) {
        printf("[ ERROR ] PT entry not present!\n");
        return;
    }
}