# Hebrew calendar on a 4-colour e-paper

> **Status: in development.** The screen layout works on the real panel, but it
> still shows **fixed sample data**. Real dates, times and daily study are not
> calculated or fetched yet.

A weekly Hebrew calendar for the wall: an ESP32-S3 drives a Good Display
7.5" black/white/yellow/red e-paper panel through [LVGL](https://lvgl.io).

The screen (800x480, landscape) shows:

- **Top line:** today's day, Hebrew date and Gregorian date
- **Left:** זמני היום (daily times) and לימוד יומי (today's daf, halacha and mishna)
- **Right:** the week Sunday–Shabbat with Hebrew and Gregorian dates, sunset,
  sunrise and events; today's row is highlighted in yellow
- **Bottom right:** כניסת / יציאת השבת in large digits, with the times for
  other cities below

The UI is designed and tested in a PC simulator:
[`hebrew_calendar_sim`](https://github.com/Rokachy/hebrew_calendar_sim).

## Hardware

| Part | Details |
|---|---|
| MCU | ESP32-S3-WROOM, no PSRAM (planned final board: ESP32-S3-MINI N4R2, 2 MB PSRAM) |
| Panel | Good Display GDEM075F52, 800x480, black/white/yellow/red, controller JD79668 |
| SPI (SPI2) | CLK 13, MOSI 14, CS 12 |
| Control | DC 11, RST 10, BUSY 9 (BUSY is active low) |

The panel only supports a full refresh, which takes about 20 s. After every
update it goes into deep sleep and is re-initialised before the next one.

## Project layout

| Path | What |
|---|---|
| `main/main.c` | starts the LVGL task, builds the calendar screen |
| `main/epaper_uc8179.c/h` | low-level panel driver (the name is historical, the chip is a JD79668) |
| `main/epd_panel.c/h` | connects the driver to the LVGL port (init / show frame) |
| `components/epaper_ui/` | **copied from the simulator repo** – the calendar UI, fonts, colour palette and the LVGL display port |
| `sdkconfig.defaults` | ESP32-S3 and LVGL settings |

Do not edit `components/epaper_ui/` here. Change the simulator, then copy it over:

```powershell
powershell -ExecutionPolicy Bypass -File C:\work\hebrew_calendar_sim\tools\sync_to_hw.ps1
```

### How the display works

LVGL draws the screen in small 16-bit strips (~38 KB). Each strip is converted
to the panel's 4 colours and packed into a 96 KB frame (2 bits per pixel), which
is sent to the panel once per update. Everything fits in internal RAM, so no
PSRAM is needed. LVGL itself allocates from the ESP heap (the UI needs about 80 KB).

## Build and flash

ESP-IDF v6.0.1. LVGL 9.6 is downloaded automatically by the component manager.

```powershell
. C:\Espressif\tools\Microsoft.v6.0.1.PowerShell_profile.ps1
idf.py build
idf.py -p COM9 flash monitor
```

The first screen appears about 20–25 s after boot.

## Designs

| Branch | Design |
|---|---|
| `new_look` | landscape (current) |
| `std_look` | the earlier portrait design |

## Still to do

- Calculate or fetch real data: Hebrew date, daily times, sunrise/sunset,
  Shabbat times, daily study and events
- Update the screen on a schedule (e.g. once a day and before Shabbat)
- Low-power operation between updates
- Move to the final ESP32-S3-MINI board

## Licence

GPL-3.0, see `LICENSE`.
