#pragma once
#include <stdbool.h>

/*
 * Keeps the ESP32's clock (RTC) set to Israel local time.
 *
 * The RTC keeps running in deep sleep, so Wi-Fi is only used when needed:
 *  - the time is not valid (first start or after power loss), or
 *  - the last internet sync is older than TIME_RESYNC_DAYS (RTC drift).
 * Wi-Fi is switched off again before returning, to free its RAM for LVGL.
 */

#define TIME_RESYNC_DAYS  30

/** Set the timezone and sync over Wi-Fi if needed. Returns true if the time is valid. */
bool time_sync_ensure(void);

/** True once the clock has been set (after 2024-01-01) */
bool time_is_valid(void);
