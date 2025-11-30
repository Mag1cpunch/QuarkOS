#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <limine.h>

#include <hardware/memory/pmm.h>
#include <hardware/memory/paging.h>
#include <hardware/requests.h>

static struct FreeBlock *free_list_head;
uint64_t hhdm;

void pmm_init() {
    struct limine_memmap_response *mm = get_memmap();
    hhdm = get_hhdm_offset();

    size_t stolen = 1;
    free_list_head = NULL;

    size_t total_pages = 0;
    size_t total_bytes = 0;
    for (size_t e = 0; e < mm->entry_count; ++e) {
        struct limine_memmap_entry *ent = mm->entries[e];
        if (ent->type != LIMINE_MEMMAP_USABLE)
            continue;

        uintptr_t p   = ent->base;
        uintptr_t end = ent->base + ent->length;

        if (stolen) {
            p     += stolen * PAGE_SIZE;
            stolen = 0;
        }

        for (; p + PAGE_SIZE <= end; p += PAGE_SIZE) {
            struct FreeBlock *blk = (void *)(hhdm + p);
            blk->next = free_list_head;
            free_list_head = blk;
            total_pages++;
            total_bytes += PAGE_SIZE;
        }
    }
    printf("[ PMM ] Total pages added: %zu (%zu bytes, %zu MiB)\n", total_pages, total_bytes, total_bytes / (1024 * 1024));
}

uintptr_t alloc_page() {
    if (!free_list_head) {
        printf("[PMM] alloc_page: Out of physical memory!\n");
        return 0;
    }
    struct FreeBlock *page = free_list_head;
    free_list_head = page->next;
    uintptr_t ret = (uintptr_t)page - hhdm;
    return ret;
}

uintptr_t alloc_pages(size_t num_pages) {
    uintptr_t first_page = 0;
    for (size_t i = 0; i < num_pages; ++i) {
        uintptr_t page = alloc_page();
        if (page == 0) {
            for (size_t j = 0; j < i; ++j) {
                free_page(first_page + j * PAGE_SIZE);
            }
            return 0;
        }
        if (i == 0) {
            first_page = page;
        }
    }
    return first_page;
}

void free_page(uintptr_t phys) {
    struct FreeBlock *b = (void *)(hhdm + phys);
    b->next = free_list_head;
    free_list_head = b;
}
