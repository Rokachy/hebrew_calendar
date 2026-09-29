/**
 * @file epd_palette.c
 *
 */

#include "epd_palette.h"

typedef struct {
    epd_color_t code;
    uint8_t r, g, b;
} epd_palette_entry_t;

static const epd_palette_entry_t epd_palette[] = {
    {EPD_BLACK,  0x00, 0x00, 0x00},
    {EPD_WHITE,  0xFF, 0xFF, 0xFF},
    {EPD_YELLOW, 0xFF, 0xFF, 0x00},
    {EPD_RED,    0xFF, 0x00, 0x00},
};

#define EPD_PALETTE_SIZE (sizeof(epd_palette) / sizeof(epd_palette[0]))

epd_color_t epd_palette_nearest(uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t best_dist = 0xFFFFFFFF;
    epd_color_t best = EPD_WHITE;

    for(uint32_t i = 0; i < EPD_PALETTE_SIZE; i++) {
        int32_t dr = (int32_t)r - epd_palette[i].r;
        int32_t dg = (int32_t)g - epd_palette[i].g;
        int32_t db = (int32_t)b - epd_palette[i].b;
        uint32_t dist = (uint32_t)(dr * dr + dg * dg + db * db);
        if(dist < best_dist) {
            best_dist = dist;
            best = epd_palette[i].code;
        }
    }

    return best;
}

void epd_palette_rgb(epd_color_t c, uint8_t * r, uint8_t * g, uint8_t * b)
{
    for(uint32_t i = 0; i < EPD_PALETTE_SIZE; i++) {
        if(epd_palette[i].code == c) {
            *r = epd_palette[i].r;
            *g = epd_palette[i].g;
            *b = epd_palette[i].b;
            return;
        }
    }
    *r = *g = *b = 0xFF;
}
