#include <string.h>

#include <hardware/requests.h>

#include <system/term.h>
#include <system/flanterm.h>
#include <system/flanterm_backends/fb.h>

static struct flanterm_context *ft_ctx;

void term_init() {
    struct limine_framebuffer_response *framebuffer_response = get_framebuffer();
    struct limine_framebuffer *framebuffer = framebuffer_response->framebuffers[0];

    ft_ctx = flanterm_fb_init(
        NULL,
        NULL,
        framebuffer->address, framebuffer->width, framebuffer->height, framebuffer->pitch,
        framebuffer->red_mask_size, framebuffer->red_mask_shift,
        framebuffer->green_mask_size, framebuffer->green_mask_shift,
        framebuffer->blue_mask_size, framebuffer->blue_mask_shift,
        NULL,
        NULL, NULL,
        NULL, NULL,
        NULL, NULL,
        NULL, 0, 0, 1,
        0, 0,
        0
    );
}

void term_writeline(const char *s) {
    flanterm_write(ft_ctx, s, strlen(s));
    flanterm_write(ft_ctx, "\n", 1);
}

void term_write(const char *s) {
    flanterm_write(ft_ctx, s, strlen(s));
}

void kputchar(char c) {
    flanterm_write(ft_ctx, &c, 1); 
}