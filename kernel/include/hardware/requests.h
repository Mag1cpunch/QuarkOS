#ifndef REQUESTS_H
#define REQUESTS_H

#include <stdint.h>

#include <limine.h>

struct limine_framebuffer_response *get_framebuffer();
struct limine_memmap_response *get_memmap();
uint64_t get_hhdm_offset();
struct limine_terminal_response *get_terminal();
struct limine_rsdp_response *get_rsdp();

#endif