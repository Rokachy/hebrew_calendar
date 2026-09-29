/**
 * @file epd_palette.h
 *
 * Colour mapping for the Good Display 7.5" 800x480 black/white/yellow/red
 * e-paper panel. Shared by the PC simulator (preview) and the ESP32-S3 port
 * (real panel), so both show exactly the same result.
 *
 * Portable: depends only on LVGL.
 */

#ifndef EPD_PALETTE_H
#define EPD_PALETTE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define EPD_WIDTH  800
#define EPD_HEIGHT 480

/** 2 bits per pixel, 4 pixels per byte, MSB first */
#define EPD_FB_SIZE (EPD_WIDTH * EPD_HEIGHT / 4)

/**
 * 2-bit codes the panel controller expects.
 * Verify against the codes your working HW driver uses and adjust if needed.
 */
typedef enum {
    EPD_BLACK  = 0x0,
    EPD_WHITE  = 0x1,
    EPD_YELLOW = 0x2,
    EPD_RED    = 0x3,
} epd_color_t;

/** Nearest panel colour for an RGB888 value */
epd_color_t epd_palette_nearest(uint8_t r, uint8_t g, uint8_t b);

/** RGB888 value that represents a panel colour (for the simulator preview) */
void epd_palette_rgb(epd_color_t c, uint8_t * r, uint8_t * g, uint8_t * b);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*EPD_PALETTE_H*/
