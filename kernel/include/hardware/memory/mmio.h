#ifndef MMIO_H
#define MMIO_H


#define MMIO_WINDOW_BASE 0xffff800000000000UL

#include <stdint.h>
#include <stddef.h>

void *map_mmio_region(uintptr_t phys, size_t size, uint64_t flags);

#endif // MMIO_H