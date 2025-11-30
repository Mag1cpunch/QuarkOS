#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <hardware/memory/heap.h>
#include <hardware/memory/pmm.h>
#include <hardware/memory/paging.h>

#include <system/term.h>

extern uint8_t _kernel_end[];

static uint64_t heap_base;
static uint64_t heap_top;
static uint64_t heap_end;

static hdr_t *free_list = NULL;

void heap_init()
{
    heap_base = ((uint64_t)_kernel_end + PAGE_SIZE-1) & ~(uint64_t)(PAGE_SIZE-1);
    heap_top  = heap_end = heap_base;

    printf("\n-----------------------------------------------\n");
    printf("[ Heap Info ]\n");
    printf("[ Kernel end: %p ]\n", _kernel_end);
    printf("[ Heap base: %#zx ]\n[ Heap top: %#zx ]\n", heap_base, heap_top);
    printf("[ Heap end address: %#zx ]\n", heap_end);
    printf("[ Heap max: %#llx ]\n", (unsigned long long)HEAP_MAX);
    printf("-----------------------------------------------\n\n");

    uintptr_t phys = alloc_page();
    mapPage((void*)heap_end, phys, PG_PRESENT | PG_WRITABLE | PG_GLOBAL | PG_NX);

    page_meta_t *pm = (page_meta_t*)heap_end;
    pm->live = 0;
    pm->flags = 0;

    heap_end += PAGE_SIZE;
}

void *kmalloc(size_t size)
{
    size = (size + 7) & ~7ULL;
    size_t total = size + HDR_SIZE;

    uint64_t required_end = heap_top + total;
    while (heap_end < required_end) {
        //printf("[ KMALLOC LOOP ] heap_end=%#zx required_end=%#zx (diff=%#zx)\n", heap_end, required_end, required_end - heap_end);
        uint64_t next_heap_end = (uint64_t)heap_end + (uint64_t)PAGE_SIZE;
        //printf("[ KMALLOC CHECK ] heap_end=%#llx + PAGE_SIZE=%#llx = next_heap_end=%#llx, HEAP_MAX=%#llx\n",
        //    (unsigned long long)heap_end, (unsigned long long)PAGE_SIZE, (unsigned long long)next_heap_end, (unsigned long long)heap_max64);
        int signed_cmp = ((int64_t)next_heap_end > (int64_t)HEAP_MAX);
        int unsigned_cmp = ((uint64_t)next_heap_end > (uint64_t)HEAP_MAX);
        if (unsigned_cmp) {
            printf("[ KMALLOC ] Heap limit reached! Max: %#llx, Next end: %#llx, Current end: %#llx\n", (unsigned long long)HEAP_MAX, (unsigned long long)next_heap_end, (unsigned long long)heap_end);
            return NULL;
        }
        uintptr_t phys = alloc_page();
        //printf("[ KMALLOC ] alloc_page() -> phys=%#zx for heap_end=%#zx\n", phys, heap_end);
        if (!phys) {
            //printf("[ KMALLOC ] alloc_page returned 0 (PMM out of memory) while allocating %zu bytes (heap_top=%#zx, heap_end=%#zx)\n", size, heap_top, heap_end);
            return NULL;
        }

        //printf("[ KMALLOC ] mapPage(virt=%#zx, phys=%#zx)\n", heap_end, phys);
        mapPage((void*)heap_end, phys,
            PG_PRESENT | PG_WRITABLE | PG_GLOBAL | PG_NX);
        extern void* getPhysicalAddress(void* virtual_address);
        void* phys_check = getPhysicalAddress((void*)heap_end);

        page_meta_t *pm = (page_meta_t*)heap_end;
        pm->live = 0; pm->flags = 0;

        heap_end += PAGE_SIZE;
        //printf("[ KMALLOC LOOP ] heap_end incremented to %#zx\n", heap_end);
    }

    hdr_t *hdr = (hdr_t*)heap_top;
    hdr->size  = total;
    hdr->free  = false;
    hdr->next  = NULL;

    heap_top += total;

    page_inc_live(hdr);

    return (void*)((uintptr_t)hdr + HDR_SIZE);
}


void kfree(void *ptr)
{
    if (!ptr) return;

    hdr_t *blk = (hdr_t*)((uintptr_t)ptr - HDR_SIZE);
    blk->free  = true;

    hdr_t *prev = NULL, *cur = free_list;
    while (cur && cur < blk) { prev = cur; cur = cur->next; }
    blk->next = cur;
    if (prev) prev->next = blk; else free_list = blk;

    if (cur && cur->free &&
        (uintptr_t)blk + blk->size == (uintptr_t)cur) {
        blk->size += cur->size;
        blk->next  = cur->next;
    }
    if (prev && prev->free &&
        (uintptr_t)prev + prev->size == (uintptr_t)blk) {
        prev->size += blk->size;
        prev->next  = blk->next;
        blk = prev;
    }

    if (page_dec_live_check_empty(blk)) {
        uintptr_t base = page_base(blk);
        unmapPage((void*)base);
        return;
    }
}



void *krealloc(void *old, size_t newsz)
{
    if (!old)                return kmalloc(newsz);
    if (!newsz) { kfree(old); return NULL; }

    hdr_t *blk = (hdr_t*)((uintptr_t)old - HDR_SIZE);
    size_t old_payload = blk->size - HDR_SIZE;

    if (newsz <= old_payload) {
        size_t excess = old_payload - newsz;
        if (excess >= HDR_SIZE + 8) {
            hdr_t *split = (void*)((uintptr_t)blk + HDR_SIZE + newsz);
            split->size = excess;
            split->free = true;
            split->next = blk->next;
            blk->size   = newsz + HDR_SIZE;
            blk->next   = split;
            blk->free   = false;
        }
        return old;
    }

    if (blk->next && blk->next->free &&
        blk->size + blk->next->size >= newsz + HDR_SIZE)
    {
        blk->size += blk->next->size;
        blk->next  = blk->next->next;
        return krealloc(old, newsz);
    }

    void *newptr = kmalloc(newsz);
    if (!newptr) return NULL;
    memcpy(newptr, old, old_payload);
    kfree(old);
    return newptr;
}

