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

/** Screen size the UI is laid out for (portrait). The display must match. */
#define UI_HOR_RES 480
#define UI_VER_RES 800

/** Build all calendar widgets on the active screen of the default display */
void ui_calendar_create(void);

/** Fill the calendar widgets with data */
void ui_calendar_update(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*UI_CALENDAR_H*/
