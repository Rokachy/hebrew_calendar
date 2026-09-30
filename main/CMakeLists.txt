idf_component_register(
    SRCS "main.c" "epaper_uc8179.c" "epd_panel.c" "time_sync.c"
    INCLUDE_DIRS "."
    REQUIRES
        esp_driver_spi
        esp_driver_gpio
        esp_timer
        freertos
        log
        heap
        epaper_ui
        esp_wifi
        esp_netif
        esp_event
        nvs_flash
)