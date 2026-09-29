#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "lvgl.h"
#include "epd_lvgl_port.h"
#include "epd_panel.h"
#include "ui_calendar.h"

static const char *TAG = "MAIN";

// LVGL rendering (fonts, BiDi) needs more stack than the 8 KB main task
#define LVGL_TASK_STACK_SIZE (16 * 1024)

static void lvgl_task(void *arg) {
    lv_init();
    epd_lvgl_port_init(&epd_panel_ops);

    ui_calendar_create();
    ui_calendar_update();

    ESP_LOGI(TAG, "UI created, free internal heap: %u bytes",
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

    while (1) {
        // Blocks for ~20 s while the panel refreshes after a UI change
        uint32_t ms = lv_timer_handler();
        if (ms == LV_NO_TIMER_READY) {
            ms = 1000;
        }
        vTaskDelay(pdMS_TO_TICKS(ms));
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "=== Hebrew calendar, GDEM075F52 ===");
    xTaskCreate(lvgl_task, "lvgl", LVGL_TASK_STACK_SIZE, NULL, 5, NULL);
}
