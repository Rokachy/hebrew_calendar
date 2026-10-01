/**
 * @file zmanim.h
 *
 * זמני היום for one day and place.
 *
 * Definitions:
 *   שעה זמנית (גר"א) 1/12 of sunrise ... sunset (sea level)
 *   עלות השחר        72 seasonal minutes (1.2 שעות זמניות גר"א) before sea-level sunrise
 *   משיכיר (ציצית)   66 seasonal minutes (1.1 שעות זמניות גר"א) before sea-level sunrise
 *   שעה זמנית (מג"א) 1/12 of (sunrise - 72 seasonal min) ... (sunset + 72 seasonal min)
 *   סו"ז ק"ש          3 seasonal hours after the start of the day (מג"א / גר"א)
 *   סו"ז תפילה        4 seasonal hours after sunrise (גר"א)
 *   חצות היום         half way between sunrise and sunset
 *   מנחה גדולה        חצות + 30 fixed minutes
 *   פלג המנחה         1.25 seasonal hours (גר"א) before sunset
 *
 * Changes made to match the family's printed calendar:
 *   - עלות השחר was the sun 16.1 degrees below the horizon (Hebcal's definition),
 *     which gave 5:18 on 20 Dec 2026; the calendar has 5:36. 72 seasonal minutes
 *     gives 5:36:38, and the calendar shows it rounded down (5:36).
 *   - The מג"א day was sunrise - 72 min ... sunset + 72 min (fixed minutes, Hebcal),
 *     which gave סו"ז ק"ש מג"א 8:31 on 20 Dec 2026; the calendar has 8:37. With 72
 *     seasonal minutes on both ends it is 8:37:31, shown 8:37.
 *   - משיכיר (ציצית ותפילין) was the sun 11.5 degrees below the horizon (Hebcal).
 *     That matched the calendar in December (5:42) but gave 4:35 on 20 Jun 2027,
 *     where the calendar has 4:17. 66 seasonal minutes fits both dates:
 *     5:41:39 -> 5:42 and 4:16:26 -> 4:17 (rounded up).
 *   - מנחה גדולה was חצות + half a seasonal hour (Hebcal), which is 35.6 min in June
 *     but only 25 min in December: 1:19 on 20 Jun 2027 (calendar 1:13) and 12:04 on
 *     20 Dec 2026 (calendar 12:09). חצות + 30 fixed minutes matches both:
 *     1:13 in June, 12:09 in December (12:08:24 rounded up).
 *   Checked unchanged against the same calendar on 20 Dec 2026: סו"ז ק"ש גר"א 9:07.
 *   The other times use Hebcal's definitions and agree with it to the second.
 */

#ifndef ZMANIM_H
#define ZMANIM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <time.h>
#include "location.h"

/** In the order they are shown */
typedef enum {
    ZMAN_ALOT,          /**< עלות השחר */
    ZMAN_MISHEYAKIR,    /**< ציצית ותפילין */
    ZMAN_SHMA_MGA,      /**< סו"ז ק"ש מג"א */
    ZMAN_SHMA_GRA,      /**< סו"ז ק"ש גר"א */
    ZMAN_TFILA_GRA,     /**< סו"ז תפילה גר"א */
    ZMAN_CHATZOT,       /**< חצות היום */
    ZMAN_MINCHA_GEDOLA, /**< מנחה גדולה */
    ZMAN_PLAG,          /**< פלג המנחה */
    ZMAN_COUNT
} zman_t;

/**
 * All zmanim of the day (Gregorian, month 1-12) at `city`, exact to the second (UTC).
 * @return false if the sun doesn't rise / set that day
 */
bool zmanim_calc(const city_t *city, int year, int month, int day, time_t out[ZMAN_COUNT]);

/**
 * Rounded to the whole minute in the careful direction: earliest-allowed times up,
 * deadlines (סוף זמן) and עלות השחר (start of a fast) down, חצות to the nearest minute.
 */
time_t zman_rounded(zman_t which, time_t exact);

/**
 * צאת הכוכבים of the day: 13.5 seasonal minutes (גר"א) after sea-level sunset,
 * rounded up. Matched to the family's printed calendar (week of 27 Sep 2026:
 * 6:45 ... 6:37). A fixed 14 minutes also fits that week; the family chose 13.5
 * seasonal minutes (not yet compared in winter / summer: 20 Dec 2026 would be
 * 4:52, a fixed 14 min 4:54). Not used for the end of Shabbat, which is 8.5
 * degrees (shabbat.h).
 * @param out  UTC time, already rounded
 */
bool zman_tzeit(const city_t *city, int year, int month, int day, time_t *out);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*ZMANIM_H*/
