#ifndef HEAP_H
#define HEAP_H

#define HEAP_MAX 0xFFFFFFFFFFFFFFFFULL

#include <stdbool.h>
#include <stddef.h>

typedef struct header {
    size_t      size;
    struct header *next;
    bool        free;
} hdr_t;

#define HDR_SIZE ((size_t)sizeof(hdr_t))

void heap_init();
void *kmalloc(size_t sz);
void kfree(void *ptr);
void *krealloc(void *old, size_t newsz);

#endif