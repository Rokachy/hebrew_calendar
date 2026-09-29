idf_component_register(
    SRCS "main.c" "epaper_uc8179.c"
    INCLUDE_DIRS "."
    REQUIRES
        esp_driver_spi
        esp_driver_gpio
        esp_timer
        freertos
        log
)