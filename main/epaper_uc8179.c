#include "epaper_uc8179.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdlib.h>
#include <string.h>

static const char *TAG = "EPD_075F52";
static spi_device_handle_t spi_handle;

#define SPI_MAX_CHUNK_BYTES 4096

static void epd_delay_ms(uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void epaper_wait_busy(void) {
    epd_delay_ms(10);

    int timeout = 0;
    // JD79668: BUSY is LOW while busy, HIGH when idle
    while (gpio_get_level(EPD_PIN_BUSY) == 0) {
        epd_delay_ms(100);
        timeout += 100;
        if (timeout % 2000 == 0) {
            ESP_LOGI(TAG, "  Busy... (%d ms)", timeout);
        }
        if (timeout > 40000) {
            ESP_LOGE(TAG, "BUSY timeout! Check BUSY wiring / panel power.");
            return;
        }
    }
    ESP_LOGI(TAG, "Ready (%d ms)", timeout);
}

static void epd_send_cmd(uint8_t cmd) {
    gpio_set_level(EPD_PIN_DC, 0);
    spi_transaction_t t = { .length = 8, .tx_buffer = &cmd };
    spi_device_polling_transmit(spi_handle, &t);
}

static void epd_send_data(uint8_t data) {
    gpio_set_level(EPD_PIN_DC, 1);
    spi_transaction_t t = { .length = 8, .tx_buffer = &data };
    spi_device_polling_transmit(spi_handle, &t);
}

static void epd_send_cmd_data(uint8_t cmd, const uint8_t *data, size_t len) {
    epd_send_cmd(cmd);
    for (size_t i = 0; i < len; i++) {
        epd_send_data(data[i]);
    }
}

static void epd_send_buf(const uint8_t *buf, size_t len) {
    gpio_set_level(EPD_PIN_DC, 1);
    size_t offset = 0;
    while (offset < len) {
        size_t chunk = (len - offset > SPI_MAX_CHUNK_BYTES) ? SPI_MAX_CHUNK_BYTES : (len - offset);
        spi_transaction_t t = { .length = chunk * 8, .tx_buffer = buf + offset };
        spi_device_polling_transmit(spi_handle, &t);
        offset += chunk;
    }
}

static void epd_hw_reset(void) {
    gpio_set_level(EPD_PIN_RST, 1);
    epd_delay_ms(20);
    gpio_set_level(EPD_PIN_RST, 0);
    epd_delay_ms(2);
    gpio_set_level(EPD_PIN_RST, 1);
    epd_delay_ms(20);
}

static void epd_panel_init(void) {
    epd_hw_reset();
    epaper_wait_busy();

    epd_send_cmd_data(0x4D, (const uint8_t[]){ 0x78 }, 1);
    epd_send_cmd_data(0x00, (const uint8_t[]){ 0x0F, 0x29 }, 2);             // PSR
    epd_send_cmd_data(0x06, (const uint8_t[]){ 0x0F, 0x8B, 0x93, 0xA1 }, 4); // Booster soft start
    epd_send_cmd_data(0x41, (const uint8_t[]){ 0x00 }, 1);                   // TSE
    epd_send_cmd_data(0x50, (const uint8_t[]){ 0x37 }, 1);                   // CDI
    epd_send_cmd_data(0x60, (const uint8_t[]){ 0x02, 0x02 }, 2);             // TCON
    epd_send_cmd_data(0x61, (const uint8_t[]){                               // Resolution
        EPD_WIDTH >> 8, EPD_WIDTH & 0xFF, EPD_HEIGHT >> 8, EPD_HEIGHT & 0xFF }, 4);
    epd_send_cmd_data(0x62, (const uint8_t[]){
        0x98, 0x98, 0x98, 0x75, 0xCA, 0xB2, 0x98, 0x7E }, 8);
    epd_send_cmd_data(0x65, (const uint8_t[]){ 0x00, 0x00, 0x00, 0x00 }, 4); // Gate/source start
    epd_send_cmd_data(0xE7, (const uint8_t[]){ 0x1C }, 1);
    epd_send_cmd_data(0xE3, (const uint8_t[]){ 0x00 }, 1);
    epd_send_cmd_data(0xE9, (const uint8_t[]){ 0x01 }, 1);
    epd_send_cmd_data(0x30, (const uint8_t[]){ 0x08 }, 1);                   // PLL

    epd_send_cmd(0x04); // Power ON
    epaper_wait_busy();
}

esp_err_t epaper_init(void) {
    esp_err_t ret;
    ESP_LOGI(TAG, "Initializing GDEM075F52 4-color panel...");

    gpio_config_t out_cfg = {
        .pin_bit_mask = (1ULL << EPD_PIN_DC) | (1ULL << EPD_PIN_RST),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&out_cfg);

    gpio_config_t busy_cfg = {
        .pin_bit_mask = (1ULL << EPD_PIN_BUSY),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&busy_cfg);

    spi_bus_config_t buscfg = {
        .mosi_io_num = EPD_PIN_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = EPD_PIN_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = SPI_MAX_CHUNK_BYTES,
    };
    ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) return ret;

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 4 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = EPD_PIN_CS,
        .queue_size = 1,
    };
    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &spi_handle);
    if (ret != ESP_OK) return ret;

    epd_panel_init();

    ESP_LOGI(TAG, "Panel initialization complete. BUSY=%d (expect 1)",
             gpio_get_level(EPD_PIN_BUSY));
    return ESP_OK;
}

esp_err_t epaper_reinit(void) {
    // After epaper_sleep() the panel only wakes up through a hardware reset;
    // SPI and GPIO are already set up by epaper_init().
    epd_panel_init();
    return ESP_OK;
}

static void epd_refresh(void) {
    ESP_LOGI(TAG, "Refreshing (takes ~20 s)...");
    epd_send_cmd(0x12);
    epd_send_data(0x00);
    epaper_wait_busy();
}

esp_err_t epaper_display_raw(const uint8_t *frame_buffer, size_t buf_size) {
    if (buf_size != EPD_BUF_SIZE) {
        ESP_LOGE(TAG, "Buffer must be %d bytes (2bpp), got %d", EPD_BUF_SIZE, (int)buf_size);
        return ESP_ERR_INVALID_SIZE;
    }
    epd_send_cmd(0x10);
    epd_send_buf(frame_buffer, buf_size);
    epd_refresh();
    return ESP_OK;
}

esp_err_t epaper_clear(void) {
    uint8_t line[EPD_WIDTH / 4];
    memset(line, 0x55, sizeof(line)); // 01 01 01 01 = 4 white pixels

    epd_send_cmd(0x10);
    for (int y = 0; y < EPD_HEIGHT; y++) {
        epd_send_buf(line, sizeof(line));
    }
    epd_refresh();
    return ESP_OK;
}

esp_err_t epaper_sleep(void) {
    epd_send_cmd(0x02); // Power OFF
    epd_send_data(0x00);
    epaper_wait_busy();
    epd_send_cmd(0x07); // Deep sleep
    epd_send_data(0xA5);
    return ESP_OK;
}

void epaper_set_pixel(uint8_t *buf, int x, int y, uint8_t color) {
    if (x < 0 || x >= EPD_WIDTH || y < 0 || y >= EPD_HEIGHT) return;
    int idx = y * (EPD_WIDTH / 4) + x / 4;
    int shift = 6 - 2 * (x % 4);
    buf[idx] = (buf[idx] & ~(0x3 << shift)) | ((color & 0x3) << shift);
}
