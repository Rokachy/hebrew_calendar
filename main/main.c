#include <stdio.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_sleep.h"
#include "esp_attr.h"
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

// Testing: wake every N seconds instead of at midnight (shows the deep-sleep wake
// path without Wi-Fi). 0 = normal operation (once a day at 00:01).
// Now: every hour, for a stability test.
#define TEST_SLEEP_SECONDS   3600

// Screen updates since the last power-on / reset. Kept in RTC memory: survives deep
// sleep, cleared by a power loss or a crash - so on the screen ("#12") it shows
// whether the device has been running without resets.
static RTC_DATA_ATTR uint32_t update_count;

static void deep_sleep_for(time_t seconds) {
    ESP_LOGI(TAG, "Deep sleep for %lld s", (long long)seconds);
    esp_sleep_enable_timer_wakeup((uint64_t)seconds * 1000000ULL);
    esp_deep_sleep_start();
}

/** Sleep until one minute after the next local midnight */
static void deep_sleep_until_tomorrow(void) {
    if (TEST_SLEEP_SECONDS > 0) {
        deep_sleep_for(TEST_SLEEP_SECONDS);
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

/** Draw today's calendar once, then deep sleep until tomorrow */
static void display_task(void *arg) {
    lv_init();
    epd_lvgl_port_init(&epd_panel_ops);

    time_t now = time(NULL);
    struct tm today;
    localtime_r(&now, &today);

    ui_calendar_create();
    ui_calendar_update(&today);

    char note[16];
    update_count++;
    snprintf(note, sizeof(note), "#%lu", (unsigned long)update_count);
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

    // Wi-Fi only when the RTC has no valid time or a resync is due
    if (!time_sync_ensure()) {
        ESP_LOGE(TAG, "No valid time, screen not updated");
        deep_sleep_for(RETRY_SLEEP_MIN * 60);
    }

    xTaskCreate(display_task, "display", LVGL_TASK_STACK_SIZE, NULL, 5, NULL);
}
