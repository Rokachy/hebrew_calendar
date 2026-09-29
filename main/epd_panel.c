#include "epd_panel.h"
#include "epaper_uc8179.h"
#include "esp_err.h"
#include <stdbool.h>

// epaper_sleep() is called after every update, so the next one needs a re-init
static bool panel_asleep = false;

static void panel_init(void) {
    ESP_ERROR_CHECK(epaper_init());
}

static void panel_show(const uint8_t *fb) {
    if (panel_asleep) {
        epaper_reinit();
    }
    epaper_display_raw(fb, EPD_BUF_SIZE);
    epaper_sleep();
    panel_asleep = true;
}

const epd_hw_ops_t epd_panel_ops = {
    .init = panel_init,
    .show = panel_show,
};
