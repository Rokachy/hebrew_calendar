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
 * All dates, times, the daf yomi, the mishna yomit and the events (holidays,
 * family dates) are calculated from it (src/cal).
 */
void ui_calendar_update(const struct tm *today);

/**
 * Extra debug text at the end of the bottom "Updated ..." line (English), e.g.
 * "#15  NTP 14/15  drift +1.8s/26.0h (+50s/30d)" from the HW while testing.
 * NULL or "" = nothing. Call after ui_calendar_create().
 */
void ui_calendar_set_note(const char *text);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*UI_CALENDAR_H*/
