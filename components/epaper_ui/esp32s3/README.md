# ESP32-S3 port

This folder is **not** built by the PC simulator. Copy it into the ESP32-S3
(ESP-IDF) project along with the portable code.

## What to copy

| From this repo      | Role                                              |
|---------------------|---------------------------------------------------|
| `src/ui/`           | Calendar screen + fonts (pure LVGL)               |
| `src/epd/`          | Panel palette / 2-bit colour codes (pure C)       |
| `esp32s3/`          | LVGL display driver for the panel                 |

Do not copy `src/main.c` or `src/hal/`; those are for the SDL simulator only.

Put all three into one ESP-IDF component next to `main/`:

```
HW_project/
├── main/
└── components/
    └── epaper_ui/
        ├── CMakeLists.txt
        ├── ui/        <- src/ui/
        ├── epd/       <- src/epd/
        └── esp32s3/   <- esp32s3/
```

`components/epaper_ui/CMakeLists.txt`:

```cmake
file(GLOB SRCS ui/*.c ui/fonts/*.c epd/*.c esp32s3/*.c)

idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS ui epd esp32s3
    REQUIRES lvgl esp_timer
)

# UI and font files include "lvgl.h" directly on ESP-IDF
target_compile_definitions(${COMPONENT_LIB} PUBLIC LV_LVGL_H_INCLUDE_SIMPLE)
```

Add `epaper_ui` to `REQUIRES` in `main/CMakeLists.txt`.

## What to write in the HW project

Fill an `epd_hw_ops_t` (see `epd_hw.h`) with `init` and `show` functions that
wrap the existing working panel driver, and pass it to `epd_lvgl_port_init()`.
They are function pointers so `epaper_ui` never links against `main`. Check that the 2-bit colour codes in
`src/epd/epd_palette.h` and the pixel order described in `epd_hw.h`
match what that driver sends to the panel.

## Startup

```c
lv_init();
epd_lvgl_port_init(&epd_panel_ops);  /* epd_hw_ops_t from the HW project */
ui_calendar_create();
ui_calendar_update();

while(1) {
    uint32_t ms = lv_timer_handler();   /* blocks during a panel refresh */
    vTaskDelay(pdMS_TO_TICKS(ms == LV_NO_TIMER_READY ? 1000 : ms));
}
```

The panel is only redrawn when something on screen changes (e.g. after
`ui_calendar_update()`), and a full 4-colour refresh takes a while.

## Orientation

The UI is portrait (480x800, `UI_HOR_RES`/`UI_VER_RES` in `ui_calendar.h`).
`EPD_ROTATION` in `epd_lvgl_port.c` rotates it onto the 800x480 panel while
packing pixels, so no extra buffer is needed. Use 90 or 270 depending on how
the panel is mounted (swap them if the image is upside down).

## Memory (no PSRAM)

- Render strip: ~38 KB (`EPD_DRAW_BUF_SIZE`)
- Panel frame buffer: 800 x 480 / 4 = 96 KB
- LVGL heap: widgets + styles, ~48-64 KB is enough for this UI

Fonts are `const`, so they stay in flash.

## lv_conf.h settings for the ESP32-S3 (differ from the simulator)

```c
#define LV_COLOR_DEPTH        16
#define LV_MEM_SIZE           (64 * 1024)   /* simulator uses 1 MB */
#define LV_USE_BIDI           1             /* Hebrew RTL */
#define LV_FONT_MONTSERRAT_14 1             /* default font for Latin labels */
#define LV_USE_THORVG         0             /* not needed, too big for no-PSRAM */
#define LV_USE_SDL            0
#define LV_USE_LOG            0
```

Use the same LVGL major version (v9) as the simulator.
