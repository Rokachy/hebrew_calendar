/**
 * @file parsha.c
 *
 * Israel schedule. Holidays that fall on Shabbat have their own reading and
 * push the weekly portions on: ראש השנה 1-2 תשרי, יום כיפור 10 תשרי, סוכות
 * 15-22 תשרי, פסח 15-21 ניסן, שבועות 6 סיון.
 *
 * The year starts with וילך (if there are two Shabbatot between ראש השנה and
 * סוכות) and האזינו, then בראשית on the Shabbat after שמחת תורה. Fixed points:
 *   - צו (non-leap year) or מצורע (leap year) on the last Shabbat before פסח
 *   - במדבר on the last Shabbat before שבועות
 *   - דברים on the last Shabbat on or before ט' באב
 *   - נצבים (or נצבים-וילך) on the last Shabbat before ראש השנה
 * Between two fixed points, pairs are joined only as far as needed to fit the
 * Shabbatot available, in this order:
 *   ויקהל-פקודי | תזריע-מצורע, אחרי מות-קדושים, בהר-בחוקותי |
 *   מטות-מסעי, חוקת-בלק | נצבים-וילך
 */

#include "parsha.h"
#include "hebrew_date.h"

#include <stdio.h>
#include <string.h>

static const char * const parshiot[] = {
    "בראשית", "נח", "לך לך", "וירא", "חיי שרה", "תולדות", "ויצא", "וישלח", "וישב", "מקץ",
    "ויגש", "ויחי", "שמות", "וארא", "בא", "בשלח", "יתרו", "משפטים", "תרומה", "תצוה",
    "כי תשא", "ויקהל", "פקודי", "ויקרא", "צו", "שמיני", "תזריע", "מצורע", "אחרי מות", "קדושים",
    "אמור", "בהר", "בחוקותי", "במדבר", "נשא", "בהעלותך", "שלח", "קרח", "חוקת", "בלק",
    "פינחס", "מטות", "מסעי", "דברים", "ואתחנן", "עקב", "ראה", "שופטים", "כי תצא", "כי תבוא",
    "נצבים", "וילך", "האזינו", "וזאת הברכה",
};
enum {
    BERESHIT = 0, VAYAKHEL = 21, TZAV = 24, TAZRIA = 26, METZORA = 27, ACHAREI = 28,
    BEHAR = 31, BAMIDBAR = 33, NASO = 34, CHUKAT = 38, MATOT = 41, DEVARIM = 43,
    VAETCHANAN = 44, NITZAVIM = 50, VAYELECH = 51, HAAZINU = 52, VEZOT_HABRACHA = 53,
};

#define MAX_SHABBATOT 60

/** Holiday reading for a Shabbat that is a holiday in Israel, NULL if none */
static const char *holiday_shabbat(int month, int day) {
    if (month == HM_TISHREI) {
        if (day == 1 || day == 2) return "ראש השנה";
        if (day == 10) return "יום כיפור";
        if (day == 15) return "סוכות";
        if (day >= 16 && day <= 21) return "חוה\"מ סוכות";
        if (day == 22) return "וזאת הברכה";
    }
    if (month == HM_NISAN) {
        if (day == 15) return "פסח";
        if (day >= 16 && day <= 20) return "חוה\"מ פסח";
        if (day == 21) return "שביעי של פסח";
    }
    if (month == HM_SIVAN && day == 6) return "שבועות";
    return NULL;
}

/** First Shabbat on or after day number `n` */
static long shabbat_on_or_after(long n) {
    return n + (6 - n % 7 + 7) % 7;
}

/** Regular (non-holiday) Shabbatot between ראש השנה and סוכות of `year` (1 or 2) */
static int shabbatot_before_sukkot(int year) {
    int count = 0;
    long first = hd_day_number_hebrew(year, HM_TISHREI, 3);
    long last = hd_day_number_hebrew(year, HM_TISHREI, 14);
    for (long s = shabbat_on_or_after(first); s <= last; s += 7) {
        if (s != hd_day_number_hebrew(year, HM_TISHREI, 10)) count++;
    }
    return count;
}

/** Number of regular Shabbatot in [from, to] (day numbers, inclusive) */
static int regular_shabbatot(long from, long to, const long *shabbat, const bool *holiday, int n) {
    int count = 0;
    for (int i = 0; i < n; i++) {
        if (shabbat[i] >= from && shabbat[i] <= to && !holiday[i]) count++;
    }
    return count;
}

/**
 * Plan the portions first..last (inclusive) over `slots` Shabbatot by joining
 * pairs (each pair given by its first portion) in the given order.
 * joined[p] = true means p and p+1 are read together.
 */
static int plan_segment(int pos, int anchor, int slots, const int *pairs, int pair_count, bool *joined) {
    int remaining = anchor - pos + 1;
    if (remaining <= slots) {
        // More Shabbatot than portions: the following portions move up, e.g. in
        // 5782 נשא was read before שבועות and מטות / מסעי separately
        return pos + slots;
    }
    int needed = remaining - slots;
    for (int i = 0; i < pair_count && needed > 0; i++) {
        if (pairs[i] >= pos && pairs[i] < anchor) {
            joined[pairs[i]] = true;
            needed--;
        }
    }
    return anchor + 1;
}

bool parsha_of_shabbat(int year, int month, int day, char *buf, size_t len) {
    long target = hd_day_number_greg(year, month, day);
    buf[0] = '\0';
    if (target % 7 != 6) return false;

    hdate_t hd = hd_from_greg(year, month, day);
    const char *special = holiday_shabbat(hd.month, hd.day);
    if (special) {
        snprintf(buf, len, "%s", special);
        return true;
    }

    int y = hd.year;
    bool leap = hd_is_leap(y);

    // All Shabbatot of the year (ראש השנה y ... the day before ראש השנה y+1)
    long year_start = hd_day_number_hebrew(y, HM_TISHREI, 1);
    long year_end = hd_day_number_hebrew(y + 1, HM_TISHREI, 1) - 1;
    long shabbat[MAX_SHABBATOT];
    bool holiday[MAX_SHABBATOT];
    int n = 0;
    for (long s = shabbat_on_or_after(year_start); s <= year_end && n < MAX_SHABBATOT; s += 7, n++) {
        shabbat[n] = s;
        holiday[n] = false;
    }
    // Mark holiday Shabbatot (Hebrew date from the day number)
    for (int i = 0; i < n; i++) {
        long offset = shabbat[i] - year_start;
        // Walk the months of the year to find the Hebrew date of shabbat[i]
        int months[13], mcount = 0;
        for (int m = HM_TISHREI; m <= (leap ? HM_ADAR_II : HM_ADAR_I); m++) months[mcount++] = m;
        for (int m = HM_NISAN; m <= HM_ELUL; m++) months[mcount++] = m;
        for (int k = 0; k < mcount; k++) {
            int dim = hd_days_in_month(months[k], y);
            if (offset < dim) {
                holiday[i] = holiday_shabbat(months[k], (int)offset + 1) != NULL;
                break;
            }
            offset -= dim;
        }
    }

    // Start of the year: וילך + האזינו, or האזינו only, between ראש השנה and סוכות
    bool vayelech_at_start = shabbatot_before_sukkot(y) == 2;
    // End of the year: וילך is read next year if next year has two Shabbatot before סוכות
    int last_portion = shabbatot_before_sukkot(y + 1) == 2 ? NITZAVIM : VAYELECH;

    // Fixed points (day numbers)
    long bereshit_from = hd_day_number_hebrew(y, HM_TISHREI, 23);
    long pesach = hd_day_number_hebrew(y, HM_NISAN, 15);
    long shavuot = hd_day_number_hebrew(y, HM_SIVAN, 6);
    long tisha_bav = hd_day_number_hebrew(y, HM_AV, 9);

    int pesach_anchor = leap ? METZORA : TZAV;
    bool joined[VEZOT_HABRACHA + 1] = { false };

    // Each section: portions from where the previous one ended up to its fixed
    // point, joining pairs only if there are fewer Shabbatot than portions
    int pos = BERESHIT;

    // 1) בראשית ... צו / מצורע, before פסח
    static const int pairs1[] = { VAYAKHEL, TAZRIA };
    int s1 = regular_shabbatot(bereshit_from, pesach - 1, shabbat, holiday, n);
    pos = plan_segment(pos, pesach_anchor, s1, pairs1, leap ? 2 : 1, joined);

    // 2) ... במדבר, not later than the Shabbat before שבועות
    static const int pairs2[] = { TAZRIA, ACHAREI, BEHAR };
    int s2 = regular_shabbatot(pesach, shavuot - 1, shabbat, holiday, n);
    pos = plan_segment(pos, BAMIDBAR, s2, pairs2, 3, joined);

    // 3) ... דברים, on the last Shabbat on or before ט' באב
    static const int pairs3[] = { MATOT, CHUKAT };
    int s3 = regular_shabbatot(shavuot, tisha_bav, shabbat, holiday, n);
    pos = plan_segment(pos, DEVARIM, s3, pairs3, 2, joined);

    // 4) ואתחנן ... נצבים / וילך, before ראש השנה
    static const int pairs4[] = { NITZAVIM };
    int s4 = regular_shabbatot(tisha_bav + 1, year_end, shabbat, holiday, n);
    plan_segment(pos, last_portion, s4, pairs4, last_portion == VAYELECH ? 1 : 0, joined);

    // Walk the Shabbatot and hand out the portions
    int next_start = vayelech_at_start ? VAYELECH : HAAZINU;
    int p = next_start;
    bool started_bereshit = false;
    for (int i = 0; i < n; i++) {
        if (holiday[i]) continue;
        if (!started_bereshit && shabbat[i] >= bereshit_from) {
            p = BERESHIT;
            started_bereshit = true;
        }
        bool pair = started_bereshit && p < VEZOT_HABRACHA && joined[p];
        if (shabbat[i] == target) {
            if (p > last_portion && started_bereshit) {
                return false;   // should not happen
            }
            if (pair) snprintf(buf, len, "%s-%s", parshiot[p], parshiot[p + 1]);
            else      snprintf(buf, len, "%s", parshiot[p]);
            return true;
        }
        p += pair ? 2 : 1;
    }
    return false;
}
