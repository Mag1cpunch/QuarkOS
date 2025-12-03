#ifndef DEFAULT_FRAMEBUFFER
#define DEFAULT_FRAMEBUFFER

#include <stdint.h>

void dfb_init(uint32_t width, uint32_t height, uint32_t pitch, void* framebuffer_addr);
void dfb_putpixel(uint32_t x, uint32_t y, uint32_t color);
void dfb_fillscreen(uint32_t color);
void dfb_swapbuffers(void);

#endif // DEFAULT_FRAMEBUFFER