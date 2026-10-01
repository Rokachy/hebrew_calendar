# Hebrew calendar on a 4-colour e-paper

> **Status: in development.** Everything on the screen is calculated from the
> date (no internet needed except to set the clock), except the daily study,
> which is still **sample data**, and the events column, which is still empty.

A weekly Hebrew calendar for the wall: an ESP32-S3 drives a Good Display
7.5" black/white/yellow/red e-paper panel through [LVGL](https://lvgl.io).

The screen (800x480, landscape) shows:

- **Top line:** today's Hebrew and Gregorian date
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
| `main/time_sync.c/h` | keeps the clock set: Wi-Fi + internet time only when needed |
| `main/wifi_secrets.h` | your Wi-Fi name and password – **not in git**, copy it from `wifi_secrets.example.h` |
| `components/epaper_ui/` | **copied from the simulator repo** – the calendar UI, fonts, colour palette, the calendar calculations (`cal/`) and the LVGL display port |
| `sdkconfig.defaults` | ESP32-S3 and LVGL settings |

Do not edit `components/epaper_ui/` here. Change the simulator, then copy it over:

```powershell
powershell -ExecutionPolicy Bypass -File C:\work\hebrew_calendar_sim\tools\sync_to_hw.ps1
```

### What is calculated

All in `components/epaper_ui/cal/`, from today's date and the fixed settings in
`cal/location.c` (Tel Aviv, plus Jerusalem, Haifa and Beer Sheva):

- Hebrew dates (incl. leap years), month names, Hebrew numerals
- Sunrise (visible, over the eastern hills) and sunset (sea level)
- זמני היום, with the definitions of the family's printed calendar
  (see `cal/zmanim.h`)
- כניסת השבת: sunset minus the city's minutes (22 / 40 / 30 / 20), rounded down;
  יציאת השבת: sun 8.5 degrees below the horizon, rounded up
- Holidays, ראש חודש and fasts (Israel), פרשת השבוע (Israel), daf yomi
- Family dates from `cal/family.h` – **not in git**; edit it in the simulator repo
  (`src/cal/family.h`, copied from `family.example.h`), then sync

The simulator repo has tests that check these against Hebcal and the printed calendar.

### How the display works

LVGL draws the screen in small 16-bit strips (~38 KB). Each strip is converted
to the panel's 4 colours and packed into a 96 KB frame (2 bits per pixel), which
is sent to the panel once per update. Everything fits in internal RAM, so no
PSRAM is needed. LVGL itself allocates from the ESP heap (the UI needs about 80 KB).

### Clock and sleep

1. At boot the ESP32 checks its internal clock (RTC), which keeps running in deep sleep.
2. Only if the time is not valid (first start, power loss) or the last sync is older
   than 30 days, it connects to Wi-Fi, gets the time from `pool.ntp.org` and turns
   Wi-Fi off again.
3. It draws the screen for today (Israel time, summer time included), then goes
   into deep sleep until 00:01.
4. Without a valid time (e.g. no Wi-Fi after a power loss) it leaves the screen
   as it is and tries again after 15 minutes.

## Build and flash

ESP-IDF v6.0.1. LVGL 9.6 is downloaded automatically by the component manager.

First create your Wi-Fi settings (once):

```powershell
copy main\wifi_secrets.example.h main\wifi_secrets.h   # then edit the name and password
```

```powershell
. C:\Espressif\tools\Microsoft.v6.0.1.PowerShell_profile.ps1
idf.py build
idf.py -p COM9 flash monitor
```

The first screen appears about 20–25 s after boot.

## Designs

`main` has the current landscape design. Earlier designs are kept as tags:

| Tag | Design |
|---|---|
| `new_look_tag` | first version of the landscape design |
| `std_look_tag` | the earlier portrait design |

## Still to do

- Daily study (daf yomi can be calculated; mishna / halacha yomit need a table)
- Holidays and events in the events column
- Measure the clock drift and tune the resync interval
- Move to the final ESP32-S3-MINI board

## Licence

GPL-3.0, see `LICENSE`.
