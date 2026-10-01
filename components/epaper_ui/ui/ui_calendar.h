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
 * Its time of day is shown at the bottom as the moment of the update.
 * All dates and times are calculated from it (src/cal); the daily study is still
 * sample data and the events column is still empty.
 */
void ui_calendar_update(const struct tm *today);

/**
 * Short extra text at the end of the bottom "עודכן" line, e.g. "#12" (an update
 * counter while testing). Left to right, so keep it to digits / Latin text.
 * NULL or "" = nothing. Call after ui_calendar_create().
 */
void ui_calendar_set_note(const char *text);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*UI_CALENDAR_H*/
