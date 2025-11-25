#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>

struct FreeBlock {
    struct FreeBlock *next;
};

void pmm_init();
uintptr_t alloc_page();
uintptr_t alloc_pages(size_t num_pages);
void free_page(uintptr_t phys);

#endif