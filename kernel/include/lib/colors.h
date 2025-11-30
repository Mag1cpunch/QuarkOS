#ifndef COLORS_H
#define COLORS_H

// Define color constants
#define COLOR_BLACK   0x000000
#define COLOR_WHITE   0xFFFFFF
#define COLOR_RED     0xFF0000
#define COLOR_GREEN   0x00FF00
#define COLOR_BLUE    0x0000FF
#define COLOR_YELLOW  0xFFFF00
#define COLOR_CYAN    0x00FFFF
#define COLOR_MAGENTA 0xFF00FF

#include <stdint.h>

uint32_t from_rgb(uint8_t r, uint8_t g, uint8_t b);
uint32_t from_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void to_rgba(uint32_t color, uint8_t* r, uint8_t* g, uint8_t* b, uint8_t* a);
void to_rgb(uint32_t color, uint8_t* r, uint8_t* g, uint8_t* b);

#endif // COLORS_H