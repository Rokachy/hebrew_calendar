/**
 * @file sun_times.h
 *
 * Sunrise, sunset and other sun angles (NOAA solar equations, ~1 minute accuracy).
 * Portable C: results are UTC time_t, convert with localtime() for display.
 */

#ifndef SUN_TIMES_H
#define SUN_TIMES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <time.h>

/** Sunrise / sunset at sea level: sun's upper edge on the horizon, with refraction */
#define SUN_ZENITH_SUNRISE  90.833

/**
 * Time the sun crosses `zenith` degrees (90 = centre on the horizon, 90 + x = x degrees
 * below it) on the given Gregorian date (month 1-12) at latitude / longitude
 * (degrees, north / east positive).
 * @param rising  true: morning (sunrise side), false: evening (sunset side)
 * @param out     UTC time
 * @return false if the sun never reaches that angle on that day
 */
bool sun_time(int year, int month, int day, double latitude, double longitude,
              double zenith, bool rising, time_t *out);

/** Days since 1970-01-01 of a Gregorian date (works without timegm) */
long days_from_civil(int year, int month, int day);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*SUN_TIMES_H*/
