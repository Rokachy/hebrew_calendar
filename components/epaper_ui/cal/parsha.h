/**
 * @file parsha.h
 *
 * פרשת השבוע (Israel schedule), calculated from the date.
 */

#ifndef PARSHA_H
#define PARSHA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

/**
 * Reading of the given Shabbat (Gregorian date, month 1-12, must be a Saturday):
 * e.g. "בראשית", "ויקהל-פקודי"; on a Shabbat that is a holiday the holiday
 * ("חוה\"מ סוכות", "שבועות", ...; "וזאת הברכה" on שמחת תורה).
 * @return false if the date is not a Saturday
 */
bool parsha_of_shabbat(int year, int month, int day, char *buf, size_t len);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*PARSHA_H*/
