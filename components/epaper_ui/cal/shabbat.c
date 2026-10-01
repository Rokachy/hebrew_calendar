/**
 * @file shabbat.c
 *
 */

#include "shabbat.h"
#include "sun_times.h"

bool shabbat_candle_lighting(const city_t *city, int year, int month, int day, time_t *out) {
    time_t sunset;
    if (!sun_time(year, month, day, city->latitude, city->longitude,
                  SUN_ZENITH_SUNRISE, false, &sunset)) {
        return false;
    }
    // Round down (better early than late). The 10 s margin covers the few seconds
    // solar formulas differ by, so a time right on a minute boundary goes to the
    // earlier minute (e.g. Jerusalem 2 Oct 2026: exactly 17:43:00 -> 17:42).
    time_t t = sunset - (time_t)city->candle_minutes * 60 - SHABBAT_SAFETY_SEC;
    *out = t - (t % 60);
    return true;
}

bool shabbat_end_rabbeinu_tam(const city_t *city, int year, int month, int day, time_t *out) {
    time_t sunset;
    if (!sun_time(year, month, day, city->latitude, city->longitude,
                  SUN_ZENITH_SUNRISE, false, &sunset)) {
        return false;
    }
    // Round up (better late than early), with the same safety margin
    time_t t = sunset + SHABBAT_RT_MINUTES * 60 + SHABBAT_SAFETY_SEC;
    *out = t % 60 ? t + (60 - t % 60) : t;
    return true;
}

bool shabbat_end(const city_t *city, int year, int month, int day, time_t *out) {
    time_t nightfall;
    if (!sun_time(year, month, day, city->latitude, city->longitude,
                  90.0 + SHABBAT_END_DEPRESSION, false, &nightfall)) {
        return false;
    }
    // Round up (better late than early), with the same safety margin
    time_t t = nightfall + SHABBAT_SAFETY_SEC;
    *out = t % 60 ? t + (60 - t % 60) : t;
    return true;
}
