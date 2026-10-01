/**
 * @file epd_lvgl_port.c
 *
 */

#include "epd_lvgl_port.h"
#include "epd_palette.h"

#include <string.h>
#include "esp_timer.h"

/*How the panel is mounted for the portrait UI (480x800):
 *  90:  panel turned clockwise, its left edge is the top
 *  270: panel turned counter-clockwise, its right edge is the top
 *  0:   landscape, no rotation (UI must then be 800x480)
 *If the picture is upside down, switch between 90 and 270.*/
#define EPD_ROTATION 0

#if EPD_ROTATION == 0
    #define EPD_LV_HOR_RES EPD_WIDTH
    #define EPD_LV_VER_RES EPD_HEIGHT
#else
    #define EPD_LV_HOR_RES EPD_HEIGHT
    #define EPD_LV_VER_RES EPD_WIDTH
#endif

/*Render strip: 800 x 480 / 20 px x 2 bytes = 38.4 KB of internal RAM*/
#define EPD_DRAW_BUF_SIZE (EPD_WIDTH * EPD_HEIGHT / 20 * 2)

LV_ATTRIBUTE_MEM_ALIGN static uint8_t draw_buf[EPD_DRAW_BUF_SIZE];

/*Whole panel image, 2 bits per pixel. Kept between refreshes so only
 *invalidated areas have to be re-rendered.*/
static uint8_t epd_fb[EPD_FB_SIZE];

static const epd_hw_ops_t * epd_hw;

/*x, y are LVGL (logical) coordinates*/
static void epd_fb_set_px(int32_t x, int32_t y, epd_color_t c)
{
#if EPD_ROTATION == 90
    int32_t px = y;
    y = EPD_HEIGHT - 1 - x;
    x = px;
#elif EPD_ROTATION == 270
    int32_t px = EPD_WIDTH - 1 - y;
    y = x;
    x = px;
#endif
    uint32_t i = (uint32_t)y * EPD_WIDTH + (uint32_t)x;
    uint8_t shift = (uint8_t)(6 - 2 * (i & 0x3));
    uint8_t * p = &epd_fb[i >> 2];
    *p = (uint8_t)((*p & ~(0x3 << shift)) | ((uint8_t)c << shift));
}

static void epd_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    const uint16_t * src = (const uint16_t *)px_map;

    for(int32_t y = area->y1; y <= area->y2; y++) {
        for(int32_t x = area->x1; x <= area->x2; x++) {
            uint16_t c = *src++;
            uint8_t r = (uint8_t)((((c >> 11) & 0x1F) * 255) / 31);
            uint8_t g = (uint8_t)((((c >> 5) & 0x3F) * 255) / 63);
            uint8_t b = (uint8_t)(((c & 0x1F) * 255) / 31);
            epd_fb_set_px(x, y, epd_palette_pixel(r, g, b, x, y));
        }
    }

    /*Push to the panel only once, after the last strip of this refresh*/
    if(lv_display_flush_is_last(disp)) {
        epd_hw->show(epd_fb);
    }

    lv_display_flush_ready(disp);
}

static uint32_t epd_tick_cb(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

lv_display_t * epd_lvgl_port_init(const epd_hw_ops_t * hw)
{
    epd_hw = hw;

    /*Start from an all-white frame*/
    uint8_t w = (uint8_t)EPD_WHITE;
    memset(epd_fb, (w << 6) | (w << 4) | (w << 2) | w, sizeof(epd_fb));

    epd_hw->init();

    lv_tick_set_cb(epd_tick_cb);

    lv_display_t * disp = lv_display_create(EPD_LV_HOR_RES, EPD_LV_VER_RES);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(disp, draw_buf, NULL, sizeof(draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, epd_flush_cb);
    lv_display_set_default(disp);

    return disp;
}
