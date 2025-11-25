#ifndef STDLIB_H
#define STDLIB_H

#define fence() __asm__ volatile ("":::"memory")

#include <stddef.h>

void *malloc(size_t size);
void free(void *ptr);
void *realloc(void *ptr, size_t size);

#endif