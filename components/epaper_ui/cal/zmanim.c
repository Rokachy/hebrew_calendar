/**
 * @file zmanim.c
 *
 * Definitions and the reasons for them: see zmanim.h.
 */

#include "zmanim.h"
#include "sun_times.h"

// עלות השחר: 72 seasonal minutes = 1.2 שעות זמניות (גר"א) before sea-level sunrise.
// (Was 16.1 degrees below the horizon; changed to match the family's calendar.)
#define ALOT_SEASONAL_HOURS    1.2
// משיכיר (ציצית ותפילין): 66 seasonal minutes = 1.1 שעות זמניות before sea-level sunrise.
// (Was 11.5 degrees below the horizon; changed to match the family's calendar.)
#define MISHEYAKIR_SEASONAL_HOURS  1.1
// The מג"א day also starts / ends 72 seasonal minutes before sunrise / after sunset.
// (Was a fixed 72 minutes, as in Hebcal; changed to match the family's calendar.)
#define MGA_SEASONAL_HOURS     1.2
// מנחה גדולה: 30 fixed minutes after חצות.
// (Was half a seasonal hour after חצות, as in Hebcal; changed to match the family's calendar.)
#define MINCHA_GEDOLA_MIN      30
// פלג המנחה: 1.25 seasonal hours (גר"א) before sunset, as in Hebcal
#define PLAG_SEASONAL_HOURS    1.25

bool zmanim_calc(const city_t *city, int year, int month, int day, time_t out[ZMAN_COUNT]) {
    time_t sunrise, sunset;
    double lat = city->latitude, lon = city->longitude;

    if (!sun_time(year, month, day, lat, lon, SUN_ZENITH_SUNRISE, true, &sunrise) ||
        !sun_time(year, month, day, lat, lon, SUN_ZENITH_SUNRISE, false, &sunset)) {
        return false;
    }

    double hour_gra = (double)(sunset - sunrise) / 12.0;
    time_t mga_start = sunrise - (time_t)(MGA_SEASONAL_HOURS * hour_gra);
    time_t mga_end = sunset + (time_t)(MGA_SEASONAL_HOURS * hour_gra);
    double hour_mga = (double)(mga_end - mga_start) / 12.0;
    time_t chatzot = sunrise + (time_t)(6 * hour_gra);

    out[ZMAN_ALOT] = sunrise - (time_t)(ALOT_SEASONAL_HOURS * hour_gra);
    out[ZMAN_MISHEYAKIR] = sunrise - (time_t)(MISHEYAKIR_SEASONAL_HOURS * hour_gra);
    out[ZMAN_SHMA_MGA] = mga_start + (time_t)(3 * hour_mga);
    out[ZMAN_SHMA_GRA] = sunrise + (time_t)(3 * hour_gra);
    out[ZMAN_TFILA_GRA] = sunrise + (time_t)(4 * hour_gra);
    out[ZMAN_CHATZOT] = chatzot;
    out[ZMAN_MINCHA_GEDOLA] = chatzot + MINCHA_GEDOLA_MIN * 60;
    out[ZMAN_PLAG] = sunset - (time_t)(PLAG_SEASONAL_HOURS * hour_gra);
    return true;
}

// צאת הכוכבים: 13.5 seasonal minutes after sunset (see zmanim.h)
#define TZEIT_SEASONAL_MIN     13.5

bool zman_tzeit(const city_t *city, int year, int month, int day, time_t *out) {
    time_t sunrise, sunset;
    if (!sun_time(year, month, day, city->latitude, city->longitude, SUN_ZENITH_SUNRISE, true, &sunrise) ||
        !sun_time(year, month, day, city->latitude, city->longitude, SUN_ZENITH_SUNRISE, false, &sunset)) {
        return false;
    }
    double hour_gra = (double)(sunset - sunrise) / 12.0;
    time_t t = sunset + (time_t)(TZEIT_SEASONAL_MIN / 60.0 * hour_gra);
    *out = t % 60 ? t + (60 - t % 60) : t;   // round up: not earlier than the real time
    return true;
}

time_t zman_rounded(zman_t which, time_t exact) {
    time_t down = exact - (exact % 60);
    switch (which) {
        case ZMAN_SHMA_MGA: case ZMAN_SHMA_GRA: case ZMAN_TFILA_GRA:
            return down;                                   // deadline: not later than the real time
        case ZMAN_ALOT:
            // Down, as in the family's calendar (5:36:38 -> 5:36). Also the careful
            // direction here: a fast day starts at עלות השחר.
            return down;
        case ZMAN_CHATZOT:
            return exact % 60 >= 30 ? down + 60 : down;    // nearest minute
        default:
            return exact % 60 ? down + 60 : down;          // earliest time: not earlier
    }
}
