#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <hardware/requests.h>
#include <hardware/memory/heap.h>

#include <drivers/video/dfb.h>

void* front_buffer;
void* back_buffer;

uint32_t fb_width;
uint32_t fb_height;
uint32_t fb_pitch;

uint32_t fb_size;

void dfb_init(uint32_t width, uint32_t height, uint32_t pitch, void* framebuffer_addr) {
    fb_width = width;
    fb_height = height;
    fb_pitch = pitch;
    fb_size = height * pitch;
    printf("[ DFB ] fb_size=%u\n", fb_size);

    if (!framebuffer_addr) {
        printf("[ DFB ERROR ] Framebuffer address is NULL!\n");
        for (;;) { __asm__("cli; hlt"); }
    }
    front_buffer = framebuffer_addr;
    back_buffer = kmalloc(fb_size);
    extern void* getPhysicalAddress(void* virtual_address);
    void* phys_check = getPhysicalAddress(back_buffer);
    printf("[ DFB ] getPhysicalAddress(back_buffer=%p) = %p\n", back_buffer, phys_check);

    if (!back_buffer) {
        printf("[ DFB ERROR ] Failed to allocate back buffer!\n");
        for (;;) { __asm__("cli; hlt"); }
    }
}

void dfb_putpixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= fb_width || y >= fb_height) {
        return;
    }
    uint32_t* pixel_addr = (uint32_t*)((uint8_t*)back_buffer + y * fb_pitch + x * 4);
    *pixel_addr = color;
}

void dfb_fillscreen(uint32_t color) {
    for (uint32_t y = 0; y < fb_height; y++) {
        for (uint32_t x = 0; x < fb_width; x++) {
            dfb_putpixel(x, y, color);
        }
    }
}

void dfb_swapbuffers(void) {
    memcpy(front_buffer, back_buffer, fb_size);
}