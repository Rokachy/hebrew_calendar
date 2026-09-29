/**
 * @file epd_hw.h
 *
 * What the LVGL port needs from the panel driver. The HW project fills an
 * epd_hw_ops_t with functions that wrap its existing, working e-paper driver
 * (SPI, BUSY/RST/DC pins, init sequence) and passes it to epd_lvgl_port_init().
 *
 * Passed as function pointers so this component never links against `main`.
 */

#ifndef EPD_HW_H
#define EPD_HW_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct {
    /** Power up and initialise the panel controller */
    void (*init)(void);

    /**
     * Send one full frame and refresh the panel. Blocks until the panel is done
     * (BUSY released), then may put the panel into deep sleep.
     * @param fb  EPD_FB_SIZE bytes, 2 bits per pixel (see epd_palette.h),
     *            4 pixels per byte, leftmost pixel in the two MSBs, rows top to bottom
     */
    void (*show)(const uint8_t * fb);
} epd_hw_ops_t;

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*EPD_HW_H*/
