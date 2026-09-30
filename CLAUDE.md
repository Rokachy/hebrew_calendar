 # Hebrew Calendar (ESP32-S3 + 7.5" 4-color e-paper)

  Goal: a Hebrew calendar shown on a Good Display 7.5" 4-color e-paper panel.

  ## Hardware
  - MCU: ESP32-S3, ESP-IDF v6.0.1 (install: `C:\esp-idf\.espressif\v6.0.1\esp-idf`), serial
  port COM9.
  - Panel: Good Display GDEM075F52, 800x480, Black/White/Yellow/Red. Controller JD79668 (not
  UC8179).
  - Pins: BUSY=9, RST=10, DC=11, CS=12, CLK=13, MOSI=14 (SPI2_HOST).

  ## Panel facts that are easy to get wrong
  - BUSY is active LOW: LOW = busy, HIGH = ready.
  - Framebuffer is 2 bits per pixel, 4 pixels per byte, first pixel in bits 7..6 -> 96,000
  bytes per frame.
  - Color codes: Black=0, White=1, Yellow=2, Red=3 (white fill byte = 0x55).
  - Only full refresh (~15-25 s). Refresh = cmd 0x12 + data 0x00, then wait for BUSY.
  - Put the panel into deep sleep after every update (0x02/0x00, wait, 0x07/0xA5); re-init
  (reset + init sequence) before the next one.

  ## Code
  - `main/epaper_uc8179.c/.h`: working low-level driver (init, `epaper_display_raw`,
  `epaper_clear`, `epaper_sleep`, `epaper_set_pixel`). The file name is historical; the chip
  is a JD79668.
  - The hardware test this came from: `C:\Projects\esp_epaper\examples\esp32_wroom_4color`.

  ## Build
  From PowerShell: `. C:\Espressif\tools\Microsoft.v6.0.1.PowerShell_profile.ps1` then
  `idf.py build` / `idf.py -p COM9 flash monitor`.

  ## Display / UI (shared with the simulator repo)
- `components/epaper_ui/` (ui/, epd/, esp32s3/) is a copy from the LVGL PC simulator
  (C:\work\hebrew_calendar_sim, GitHub Rokachy/hebrew_calendar_sim). Edit it there, then run
  `powershell -ExecutionPolicy Bypass -File C:\work\hebrew_calendar_sim\tools\sync_to_hw.ps1`.
  Never edit it here: the sync overwrites it.
- ESP32-S3-WROOM has no PSRAM (final board: ESP32-S3-MINI N4R2, 2 MB quad PSRAM).
  Keep large buffers in internal RAM, no full-screen RGB buffers. LVGL renders in
  ~38 KB RGB565 strips and packs them into a 96 KB 2bpp frame.
- UI size is `UI_HOR_RES`/`UI_VER_RES` in `ui_calendar.h`; `EPD_ROTATION` in
  `epd_lvgl_port.c` must match (0 = landscape 800x480, 90/270 = portrait). Branch
  `std_look` = portrait design (270), `new_look` = landscape design (0).
- LVGL <-> panel: `main/epd_panel.c` fills an `epd_hw_ops_t` (init/show) passed to
  `epd_lvgl_port_init()`. Function pointers, so epaper_ui never links against main.
- LVGL 9.6 via component manager, configured in sdkconfig (no lv_conf.h): colour depth 16,
  BiDi on, ThorVG off, LVGL uses the ESP heap (CLIB malloc; the landscape UI needs
  ~80 KB, too much for a fixed 64 KB pool), examples/demos off. See sdkconfig.defaults.
