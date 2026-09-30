/**
 * @file location.c
 *
 * Cities and their Shabbat customs.
 *
 * Candle lighting (כניסת השבת) is a fixed number of minutes before that
 * city's own sea-level sunset on Friday, by local custom:
 *   Tel Aviv    22 minutes
 *   Jerusalem   40 minutes
 *   Haifa       30 minutes
 *   Beer Sheva  20 minutes
 *
 * End of Shabbat (יציאת השבת) is the same rule everywhere: צאת הכוכבים, the sun
 * 8.5 degrees below the horizon on Saturday evening at that city
 * (SHABBAT_END_DEPRESSION in shabbat.h).
 *
 * Sunrise in the week table is the visible sunrise (הנץ הנראה): the sun appears
 * over the hills east of the city, a few minutes after sea-level sunrise.
 * east_horizon is how high those hills rise above a flat horizon, in degrees.
 * Tel Aviv 0.87: fitted to the family's printed calendar
 * (20 Jun 2027 sunrise 5:39, 20 Dec 2026 sunrise 6:42).
 * Sunset is over the sea, so it stays at sea level.
 */

#include "location.h"
#include "sun_times.h"

bool city_sunrise_sunset(const city_t *city, int year, int month, int day, bool rising, time_t *out) {
    double zenith = SUN_ZENITH_SUNRISE - (rising ? city->east_horizon : 0.0);
    return sun_time(year, month, day, city->latitude, city->longitude, zenith, rising, out);
}

const city_t loc_main = {
    .name = "תל אביב", .short_name = "ת\"א",
    .latitude = 32.0853, .longitude = 34.7818,
    .candle_minutes = 22,
    .east_horizon = 0.87,
};

const city_t loc_cities[LOC_CITY_COUNT] = {
    { .name = "ירושלים", .short_name = "י-ם",
      .latitude = 31.7780, .longitude = 35.2350, .candle_minutes = 40 },
    { .name = "חיפה", .short_name = "חיפה",
      .latitude = 32.7940, .longitude = 34.9896, .candle_minutes = 30 },
    { .name = "באר שבע", .short_name = "ב\"ש",
      .latitude = 31.2520, .longitude = 34.7915, .candle_minutes = 20 },
};
