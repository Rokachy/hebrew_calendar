/**
 * @file location.h
 *
 * Where the calendar is calculated for. The values are in location.c.
 */

#ifndef LOCATION_H
#define LOCATION_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *name;         /**< full name, e.g. "תל אביב" */
    const char *short_name;   /**< for narrow places, e.g. "י-ם" */
    double latitude;          /**< degrees north */
    double longitude;         /**< degrees east */
    int candle_minutes;       /**< Shabbat candle lighting: minutes before sunset */
    double east_horizon;      /**< degrees the eastern hills rise above a flat horizon (visible sunrise) */
} city_t;

/** Main location: sunrise/sunset, daily times and the big Shabbat times */
extern const city_t loc_main;

/** Other cities shown under the big Shabbat times, first one on the right */
#define LOC_CITY_COUNT 3
extern const city_t loc_cities[LOC_CITY_COUNT];

#include <stdbool.h>
#include <time.h>

/**
 * Sunrise / sunset as shown in the week table: visible sunrise over the eastern
 * hills (east_horizon), sea-level sunset. Gregorian date, month 1-12; UTC result.
 */
bool city_sunrise_sunset(const city_t *city, int year, int month, int day, bool rising, time_t *out);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LOCATION_H*/
