#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "epaper_uc8179.h"

static const char *TAG = "MAIN";

static void draw_color_test(void) {
    uint8_t *buf = malloc(EPD_BUF_SIZE);
    if (!buf) {
        ESP_LOGE(TAG, "Not enough memory for frame buffer!");
        return;
    }

    // Four vertical bands: BLACK | WHITE | YELLOW | RED
    for (int y = 0; y < EPD_HEIGHT; y++) {
        for (int x = 0; x < EPD_WIDTH; x++) {
            uint8_t c = x < 200 ? EPD_COLOR_BLACK
                      : x < 400 ? EPD_COLOR_WHITE
                      : x < 600 ? EPD_COLOR_YELLOW
                      :           EPD_COLOR_RED;
            epaper_set_pixel(buf, x, y, c);
        }
    }

    ESP_LOGI(TAG, "Sending 4-color test pattern...");
    epaper_display_raw(buf, EPD_BUF_SIZE);
    ESP_LOGI(TAG, "Done! Expect: BLACK | WHITE | YELLOW | RED");

    free(buf);
}

void app_main(void) {
    ESP_LOGI(TAG, "=== Good Display GDEM075F52 4-color test ===");

    ESP_ERROR_CHECK(epaper_init());

    ESP_LOGI(TAG, "Clearing screen...");
    epaper_clear();

    vTaskDelay(pdMS_TO_TICKS(1000));

    draw_color_test();

    ESP_LOGI(TAG, "Entering deep sleep.");
    epaper_sleep();
}
