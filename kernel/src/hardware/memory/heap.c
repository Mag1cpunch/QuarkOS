#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#include <hardware/memory/heap.h>
#include <hardware/memory/pmm.h>
#include <hardware/memory/paging.h>

#include <system/term.h>

extern uint8_t _kernel_end[];

static uintptr_t heap_base;
static uintptr_t heap_top;
static uintptr_t heap_end;

static hdr_t *free_list = NULL;

void heap_init()
{
    heap_base = ((uintptr_t)_kernel_end + PAGE_SIZE-1) & ~(PAGE_SIZE-1);
    heap_top  = heap_end = heap_base;

    printf("\n-----------------------------------------------\n");
    printf("[ Heap Info ]\n");
    printf("[ Heap base: %#zx ]\n[ Heap top: %#zx ]\n", heap_base, heap_top);
    printf("[ Heap end address: %#zx ]\n", heap_end);
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

    while (heap_top + total > heap_end) {
        if (heap_end + PAGE_SIZE > HEAP_MAX) return NULL;
        uintptr_t phys = alloc_page();
        if (!phys) return NULL;

        mapPage((void*)heap_end, phys,
                PG_PRESENT | PG_WRITABLE | PG_GLOBAL | PG_NX);

        page_meta_t *pm = (page_meta_t*)heap_end;
        pm->live = 0; pm->flags = 0;

        heap_end += PAGE_SIZE;
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

