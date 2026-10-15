idf_component_register(
    SRCS "main.c" "epaper_uc8179.c" "epd_panel.c" "time_sync.c" "user_data.c"
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

# Private data (Wi-Fi, family dates): user_data.csv (not in git) -> the "userdata"
# NVS partition, written by `idf.py flash` (only the data: `idf.py userdata-flash`).
# After creating user_data.csv run `idf.py reconfigure`.
idf_build_get_property(project_dir PROJECT_DIR)
if(EXISTS ${project_dir}/user_data.csv)
    nvs_create_partition_image(userdata ${project_dir}/user_data.csv FLASH_IN_PROJECT)
else()
    message(WARNING "user_data.csv not found: no Wi-Fi and no family dates. "
                    "Copy user_data.example.csv to user_data.csv and fill it in.")
endif()
