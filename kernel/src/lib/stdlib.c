#include <stdlib.h>
#include <stddef.h>

#include <hardware/memory/heap.h>

void *malloc(size_t size) {
    kmalloc(size);
}
void free(void *ptr) {
    kfree(ptr);
}
void *realloc(void *ptr, size_t size) {
    krealloc(ptr, size);
}