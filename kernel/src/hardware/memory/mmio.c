#include <stdint.h>
#include <stddef.h>

#include <hardware/memory/mmio.h>
#include <hardware/memory/paging.h>

static uintptr_t next_mmio_virt = MMIO_WINDOW_BASE;

void *map_mmio_region(uintptr_t phys, size_t size, uint64_t flags) {
    void *virt = (void *)next_mmio_virt;
    size_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    for (size_t i = 0; i < pages; ++i) {
        mapPage((void *)(next_mmio_virt + i * PAGE_SIZE), (void *)(phys + i * PAGE_SIZE), flags);
    }
    next_mmio_virt += pages * PAGE_SIZE;
    return virt;
}