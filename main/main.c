#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_sleep.h"
#include "esp_attr.h"
#include "soc/rtc.h"
#include "lvgl.h"
#include "epd_lvgl_port.h"
#include "epd_panel.h"
#include "ui_calendar.h"
#include "time_sync.h"

static const char *TAG = "MAIN";

// LVGL rendering (fonts, BiDi) needs more stack than the 8 KB main task
#define LVGL_TASK_STACK_SIZE (16 * 1024)

// No valid time (e.g. Wi-Fi down after a power loss): try again after this
#define RETRY_SLEEP_MIN      15

// Testing: wake every N seconds instead of at midnight. 0 = normal operation
// (once a day at 00:01). Now: every hour, on the full hour of the ESP32's own
// clock (10:00:00, 11:00:00, ...), for a clock-drift test: the internet time is
// only fetched once, the wake log line ("Wake 11:00:00.012") is compared with
// the PC clock in the UART log.
#define TEST_SLEEP_SECONDS   3600

// Screen updates since the last power-on / reset. Kept in RTC memory: survives deep
// sleep, cleared by a power loss or a crash - so on the screen ("#12") it shows
// whether the device has been running without resets.
static RTC_DATA_ATTR uint32_t update_count;

// Own-clock time (us since 1970) the last deep sleep was meant to end; 0 = none
static RTC_DATA_ATTR int64_t planned_wake_us;
// How late this wake was by the own clock (boot time + timer error), for the note
static int32_t wake_late_ms;
static bool wake_late_valid;

static int64_t clock_now_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

/** Log the own-clock wake time and how far it is from the planned wake */
static void log_wake_time(void) {
    int64_t now_us = clock_now_us();
    time_t now = (time_t)(now_us / 1000000);
    struct tm local;
    localtime_r(&now, &local);
    char buf[16];
    strftime(buf, sizeof(buf), "%H:%M:%S", &local);
    if (planned_wake_us > 0) {
        wake_late_ms = (int32_t)((now_us - planned_wake_us) / 1000);
        wake_late_valid = true;
        ESP_LOGI(TAG, "Wake %s.%03d by own clock, %+ld ms after the planned wake", buf,
                 (int)(now_us % 1000000 / 1000), (long)wake_late_ms);
    } else {
        ESP_LOGI(TAG, "Wake %s.%03d by own clock", buf, (int)(now_us % 1000000 / 1000));
    }
    planned_wake_us = 0;
}

static void deep_sleep_for(time_t seconds) {
    ESP_LOGI(TAG, "Deep sleep for %lld s", (long long)seconds);
    esp_sleep_enable_timer_wakeup((uint64_t)seconds * 1000000ULL);
    esp_deep_sleep_start();
}

/**
 * Sleep until the next multiple of `period` seconds on the own clock. The sleep is
 * measured from now, so the time spent awake (Wi-Fi, ~20 s panel refresh) is not
 * added to the period. Local time is UTC + whole hours, so 3600 wakes on the hour.
 */
static void deep_sleep_until_next(time_t period) {
    int64_t period_us = (int64_t)period * 1000000;
    int64_t now_us = clock_now_us();
    int64_t wake_us = (now_us / period_us + 1) * period_us;
    if (wake_us - now_us < 60 * 1000000LL) {
        wake_us += period_us;   // too close: skip to the one after
    }
    planned_wake_us = wake_us;
    ESP_LOGI(TAG, "Deep sleep for %lld.%03lld s", (long long)((wake_us - now_us) / 1000000),
             (long long)((wake_us - now_us) % 1000000 / 1000));
    esp_sleep_enable_timer_wakeup((uint64_t)(wake_us - now_us));
    esp_deep_sleep_start();
}

/** Sleep until one minute after the next local midnight */
static void deep_sleep_until_tomorrow(void) {
    if (TEST_SLEEP_SECONDS > 0) {
        deep_sleep_until_next(TEST_SLEEP_SECONDS);
    }
    time_t now = time(NULL);
    struct tm wake;
    localtime_r(&now, &wake);
    wake.tm_mday += 1;
    wake.tm_hour = 0;
    wake.tm_min = 1;
    wake.tm_sec = 0;
    wake.tm_isdst = -1;
    time_t wake_at = mktime(&wake);
    deep_sleep_for(wake_at > now ? wake_at - now : 60);
}

/**
 * Debug text for the bottom line (English):
 *   "#15  NTP 14/15  drift +1.8s/26.0h (+50s/30d)"
 * update counter, internet-time answers / tries, measured clock drift and the
 * same drift scaled to 30 days. Integers only (no float formatting needed).
 */
static void format_debug_note(char *buf, size_t len) {
    time_stats_t st;
    time_sync_get_stats(&st);
    int n = snprintf(buf, len, "#%lu  NTP %lu/%lu", (unsigned long)update_count,
                     (unsigned long)st.ok, (unsigned long)st.tries);
    if (st.drift_valid && st.hours_x10 > 0 && n > 0 && (size_t)n < len) {
        long tenths = labs((long)st.drift_ms) / 100;                          // drift in 0.1 s
        long month_s = labs((long)((int64_t)st.drift_ms * 7200 / st.hours_x10 / 1000));
        char sign = st.drift_ms < 0 ? '-' : '+';
        n += snprintf(buf + n, len - n, "  drift %c%ld.%lds/%ld.%ldh (%c%lds/30d)",
                      sign, tenths / 10, tenths % 10, (long)st.hours_x10 / 10, (long)st.hours_x10 % 10,
                      sign, month_s);
    }
    if (wake_late_valid && n > 0 && (size_t)n < len) {
        snprintf(buf + n, len - n, "  wake %+ldms", (long)wake_late_ms);
    }
}

/** Draw today's calendar once, then deep sleep until tomorrow */
static void display_task(void *arg) {
    lv_init();
    epd_lvgl_port_init(&epd_panel_ops);

    time_t now = time(NULL);
    struct tm today;
    localtime_r(&now, &today);

    ui_calendar_create();
    ui_calendar_update(&today);

    char note[96];
    update_count++;
    format_debug_note(note, sizeof(note));
    ui_calendar_set_note(note);

    ESP_LOGI(TAG, "Update %s, free internal heap: %u bytes", note,
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

    // Render and send to the panel now (blocks ~20 s for the panel refresh)
    lv_refr_now(NULL);

    deep_sleep_until_tomorrow();
}

void app_main(void) {
    // Bit mask of esp_sleep_wakeup_cause_t; 0 = power-on / reset, timer = deep-sleep wake
    ESP_LOGI(TAG, "=== Hebrew calendar, GDEM075F52 === (wake causes 0x%lx)",
             (unsigned long)esp_sleep_get_wakeup_causes());
    ESP_LOGI(TAG, "RTC slow clock: %s", rtc_clk_slow_src_get() == SOC_RTC_SLOW_CLK_SRC_XTAL32K
                                            ? "32.768 kHz crystal" : "internal RC (NOT the crystal)");

    // Wi-Fi only when the RTC has no valid time or a resync is due
    if (!time_sync_ensure()) {
        ESP_LOGE(TAG, "No valid time, screen not updated");
        deep_sleep_for(RETRY_SLEEP_MIN * 60);
    }

    // Before anything slow: the wake time by the own clock, to compare with the PC
    log_wake_time();

    xTaskCreate(display_task, "display", LVGL_TASK_STACK_SIZE, NULL, 5, NULL);
}
