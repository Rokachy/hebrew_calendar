#pragma once
#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

/*
 * Good Display GDEM075F52 - 7.5" 800x480 4-color (Black/White/Yellow/Red)
 * Controller: JD79668 (NOT UC8179 - different init, 2bpp, BUSY active LOW)
 */

// ESP32-S3 GPIO mapping
#define EPD_PIN_BUSY     9
#define EPD_PIN_RST      10
#define EPD_PIN_DC       11
#define EPD_PIN_CS       12
#define EPD_PIN_CLK      13
#define EPD_PIN_MOSI     14

#define EPD_WIDTH        800
#define EPD_HEIGHT       480

// 2 bits per pixel, 4 pixels per byte, first pixel in bits 7..6
#define EPD_BUF_SIZE     ((EPD_WIDTH * EPD_HEIGHT) / 4)

// 2-bit color codes
#define EPD_COLOR_BLACK   0x0
#define EPD_COLOR_WHITE   0x1
#define EPD_COLOR_YELLOW  0x2
#define EPD_COLOR_RED     0x3

esp_err_t epaper_init(void);
esp_err_t epaper_display_raw(const uint8_t *frame_buffer, size_t buf_size);
esp_err_t epaper_clear(void);
esp_err_t epaper_sleep(void);
void      epaper_wait_busy(void);
void      epaper_set_pixel(uint8_t *buf, int x, int y, uint8_t color);
