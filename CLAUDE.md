 # Hebrew Calendar (ESP32-S3 + 7.5" 4-color e-paper)

  Goal: a Hebrew calendar shown on a Good Display 7.5" 4-color e-paper panel.

  ## Hardware
  - MCU: ESP32-S3, ESP-IDF v6.1 (install: `C:\esp-idf\.espressif\esp-idf-v6.1`). Boards: ESP32-S3-WROOM
  dev board (serial port COM9) and Waveshare ESP32-S3-Zero (native USB, its own COM port).
  One build runs on both.
  - Panel: Good Display GDEM075F52, 800x480, Black/White/Yellow/Red. Controller JD79668 (not
  UC8179).
  - Pins: BUSY=7, RST=8, DC=9, CS=10, CLK=11, MOSI=12 (SPI2_HOST), same on both boards.

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
  From PowerShell: `. C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1` then
  `idf.py build` / `idf.py -p COM9 flash monitor`.

  ## Display / UI (shared with the simulator repo)
- `components/epaper_ui/` (ui/, epd/, esp32s3/) is a copy from the LVGL PC simulator
  (C:\work\hebrew_calendar_sim, GitHub Rokachy/hebrew_calendar_sim). Edit it there, then run
  `powershell -ExecutionPolicy Bypass -File C:\work\hebrew_calendar_sim\tools\sync_to_hw.ps1`.
  Never edit it here: the sync overwrites it.
- No PSRAM is used: the WROOM has none, the ESP32-S3-Zero's 2 MB stays off (not needed,
  and one build keeps running on both). The Zero has no 32.768 kHz crystal: ESP-IDF falls
  back to the internal RC (~3 min/day drift), fine on mains power. Keep large buffers in internal RAM, no full-screen RGB buffers. LVGL renders in
  ~38 KB RGB565 strips and packs them into a 96 KB 2bpp frame.
- UI size is `UI_HOR_RES`/`UI_VER_RES` in `ui_calendar.h`; `EPD_ROTATION` in
  `epd_lvgl_port.c` must match (0 = landscape 800x480, 90/270 = portrait). `main` =
  landscape design (0); tag `std_look_tag` = old portrait design (270).
- LVGL <-> panel: `main/epd_panel.c` fills an `epd_hw_ops_t` (init/show) passed to
  `epd_lvgl_port_init()`. Function pointers, so epaper_ui never links against main.
- LVGL 9.6 via component manager, configured in sdkconfig (no lv_conf.h): colour depth 16,
  BiDi on, ThorVG off, LVGL uses the ESP heap (CLIB malloc; the landscape UI needs
  ~80 KB, too much for a fixed 64 KB pool), examples/demos off. See sdkconfig.defaults.

## Clock / power
- `main/time_sync.c`: Israel TZ; Wi-Fi + SNTP only if the RTC time is invalid or the last
  sync (RTC_DATA_ATTR) is older than TIME_RESYNC_DAYS. Wi-Fi is deinitialised before LVGL.
- `main/main.c` draws once (`lv_refr_now`), then deep sleeps until 00:01; no valid time ->
  screen untouched, retry in 15 min.
- Private data (Wi-Fi name/password, family dates) lives in the `userdata` NVS partition
  (`partitions.csv`), image built from `user_data.csv` (gitignored, never commit it;
  template `user_data.example.csv`) and written by `idf.py flash` / `idf.py userdata-flash`,
  or without a build by `flash_user_data.bat [PORT]` (nvs_partition_gen + esptool).
  `main/user_data.c` reads it at boot and passes the family dates to the UI via
  `family_dates_set()` (the HW build defines FAMILY_DATES_RUNTIME, family.h is not synced).
