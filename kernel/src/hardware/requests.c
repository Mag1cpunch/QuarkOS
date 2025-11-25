#include <limine.h>
#include <stddef.h>
#include <stdint.h>

#include <hardware/requests.h>

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

static volatile struct limine_terminal_request terminal_request = {
    .id = LIMINE_TERMINAL_REQUEST,
    .revision = 0
};

static volatile struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0
};

struct limine_framebuffer_response *get_framebuffer() {
    if (framebuffer_request.response != NULL && framebuffer_request.response->framebuffer_count != 0)
        return framebuffer_request.response;
    return NULL;
}

struct limine_memmap_response *get_memmap() {
    if (memmap_request.response != NULL && memmap_request.response->entry_count != 0)
        return memmap_request.response;
    return NULL;
}

uint64_t get_hhdm_offset() {
    if (hhdm_request.response != NULL && hhdm_request.response->offset != 0)
        return hhdm_request.response->offset;
    return NULL;
}

struct limine_terminal_response *get_terminal() {
    if (terminal_request.response != NULL && terminal_request.response->terminal_count != 0)
        return terminal_request.response;
    return NULL;
}

struct limine_rsdp_response *get_rsdp() {
    if (rsdp_request.response != NULL && rsdp_request.response->address != 0)
        return rsdp_request.response;
    return NULL;
}