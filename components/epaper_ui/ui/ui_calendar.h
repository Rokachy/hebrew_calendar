/**
 * @file ui_calendar.h
 *
 * Weekly calendar screen. Portable: depends only on LVGL, so it builds the
 * same in the PC simulator and on the ESP32-S3.
 */

#ifndef UI_CALENDAR_H
#define UI_CALENDAR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>

/** Screen size the UI is laid out for (landscape). The display must match. */
#define UI_HOR_RES 800
#define UI_VER_RES 480

/** Build all calendar widgets on the active screen of the default display */
void ui_calendar_create(void);

/**
 * Fill the calendar widgets for the week that contains `today` (local time).
 * Gregorian dates, the top date line and the "today" highlight come from it;
 * the Hebrew dates, times and study are still sample data.
 */
void ui_calendar_update(const struct tm *today);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*UI_CALENDAR_H*/
