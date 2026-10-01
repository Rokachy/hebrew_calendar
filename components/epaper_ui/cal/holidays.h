/**
 * @file holidays.h
 *
 * Jewish holidays, fasts, ראש חודש and Israeli national days of a date
 * (Israel calendar: one day of Yom Tov, Simchat Torah on Shemini Atzeret).
 * Calculated from the Hebrew date, including the rules that move a day when it
 * would fall on Shabbat (fasts) or next to it (Israeli national days).
 */

#ifndef HOLIDAYS_H
#define HOLIDAYS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

/**
 * Events of the given Gregorian date (month 1-12), in Hebrew, e.g. "יום כיפור",
 * "חנוכה ג'", "ראש חודש חשוון", "חנוכה ו', ר\"ח" (two events on one day).
 * @return false (and an empty string) if there is no event
 */
bool holiday_events(int year, int month, int day, char *buf, size_t len);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*HOLIDAYS_H*/
