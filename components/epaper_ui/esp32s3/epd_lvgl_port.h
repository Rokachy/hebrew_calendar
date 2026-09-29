/**
 * @file epd_lvgl_port.h
 *
 * LVGL display driver for the 7.5" 4-colour e-paper on ESP32-S3.
 * Works without PSRAM: LVGL renders in small RGB565 strips which are packed
 * into a 96 KB 2-bpp frame buffer in internal RAM.
 */

#ifndef EPD_LVGL_PORT_H
#define EPD_LVGL_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "epd_hw.h"

/**
 * Create the LVGL display for the panel and set it as default.
 * Call after lv_init(). Also initialises the panel via hw->init().
 * @param hw  panel driver functions; must stay valid (e.g. static const)
 */
lv_display_t * epd_lvgl_port_init(const epd_hw_ops_t * hw);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*EPD_LVGL_PORT_H*/
