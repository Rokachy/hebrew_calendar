/**
 * @file family_dates.c
 *
 */

#include "family_dates.h"

#include <string.h>

// The private list (not in git). Without it the calendar simply has no family dates.
#if __has_include("family.h")
    #include "family.h"
#else
    #pragma message("src/cal/family.h not found: no family dates. Copy family.example.h to family.h")
    static const family_date_t family_dates[] = { { 0, 0, NULL } };
#endif

#define FAMILY_COUNT (sizeof(family_dates) / sizeof(family_dates[0]))

/** The month after `month` in `year` */
static int next_month(int month, int year) {
    if (month == HM_ADAR_II) return HM_NISAN;
    if (month == HM_ADAR_I) return hd_is_leap(year) ? HM_ADAR_II : HM_NISAN;
    if (month == HM_ELUL) return HM_TISHREI;
    return month + 1;
}

bool family_date_matches(int month, int day, const hdate_t *today) {
    bool leap = hd_is_leap(today->year);

    if (month == FAMILY_ADAR) month = leap ? HM_ADAR_II : HM_ADAR_I;
    else if (month == HM_ADAR_II && !leap) month = HM_ADAR_I;   // born in אדר ב', normal year

    if (day == 30 && hd_days_in_month(month, today->year) == 29) {
        month = next_month(month, today->year);
        day = 1;
    }
    return today->month == month && today->day == day;
}

bool family_events(int year, int month, int day, char *buf, size_t len) {
    hdate_t hd = hd_from_greg(year, month, day);
    buf[0] = '\0';
    for (size_t i = 0; i < FAMILY_COUNT; i++) {
        const family_date_t *f = &family_dates[i];
        if (f->text == NULL || f->day == 0) continue;
        if (family_date_matches(f->month, f->day, &hd)) {
            if (buf[0]) strncat(buf, ", ", len - strlen(buf) - 1);
            strncat(buf, f->text, len - strlen(buf) - 1);
        }
    }
    return buf[0] != '\0';
}
