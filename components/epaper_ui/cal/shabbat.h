/**
 * @file shabbat.h
 *
 * Shabbat times for a city.
 */

#ifndef SHABBAT_H
#define SHABBAT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <time.h>
#include "location.h"

/** Seconds taken off before rounding Shabbat entry down (calculation tolerance) */
#define SHABBAT_SAFETY_SEC 10

/**
 * Candle lighting on the given Friday (Gregorian, month 1-12): the city's
 * candle_minutes before its sunset, rounded down to the whole minute (never late).
 * @param out  UTC time
 */
bool shabbat_candle_lighting(const city_t *city, int year, int month, int day, time_t *out);

/** צאת הכוכבים / end of Shabbat: sun this many degrees below the horizon */
#define SHABBAT_END_DEPRESSION 8.5

/**
 * End of Shabbat on the given Saturday (Gregorian, month 1-12): nightfall, sun
 * SHABBAT_END_DEPRESSION degrees below the horizon at the city, rounded up to the
 * whole minute (never early).
 * @param out  UTC time
 */
bool shabbat_end(const city_t *city, int year, int month, int day, time_t *out);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*SHABBAT_H*/
