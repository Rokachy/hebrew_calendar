#pragma once
#include <stdbool.h>
#include <stdint.h>

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

/*
 * Drift statistics (testing): get the internet time and compare it with the
 * ESP32's own clock WITHOUT setting the clock, so the difference keeps growing by
 * the clock's drift since it was last set. Counts every attempt.
 * Returns false if the clock was just set or there was no answer.
 */
bool time_sync_measure_drift(void);

typedef struct {
    uint32_t tries;       /**< internet time requests since power-on */
    uint32_t ok;          /**< ... that got an answer */
    bool drift_valid;     /**< a drift measurement exists */
    int32_t drift_ms;     /**< internet time - own clock (+ = own clock is slow) */
    int32_t hours_x10;    /**< hours since the clock was set, x10 */
} time_stats_t;

void time_sync_get_stats(time_stats_t *st);
