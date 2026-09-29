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
- `ui/`, `epd/`, `esp32s3/` are copied from the LVGL PC simulator
  (C:\Projects\lv_port_pc_vscode). Edit them there and copy them back,
  not here, or the two projects drift apart.
- Panel: Good Display 7.5" 800x480 black/white/yellow/red, full refresh only.
- ESP32-S3-WROOM has no PSRAM: keep large buffers in internal RAM, no full-screen
  RGB buffers. LVGL renders in partial strips and packs into a 96 KB 2bpp frame.
- `epd_hw.h` is the only link between LVGL and the panel driver
  (`epd_hw_init`, `epd_hw_show`).
- lv_conf.h: LV_COLOR_DEPTH 16, LV_USE_BIDI 1, LV_USE_THORVG 0, LV_MEM_SIZE ~64 KB.