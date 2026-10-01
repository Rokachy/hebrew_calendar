/**
 * @file holidays.c
 *
 * Rules (Israel):
 *   ראש השנה 1-2 תשרי, צום גדליה 3 תשרי (Shabbat -> Sunday 4),
 *   יום כיפור 10 תשרי (never moved, also on Shabbat),
 *   סוכות 15, חוה"מ 16-20, הושענא רבה 21, שמחת תורה 22 תשרי,
 *   חנוכה 25 כסלו + 8 days, עשרה בטבת 10 טבת, ט"ו בשבט 15 שבט,
 *   תענית אסתר 13 אדר (Shabbat -> Thursday 11), פורים 14, שושן פורים 15 אדר
 *   (אדר ב' in a leap year; פורים קטן 14 אדר א'),
 *   פסח 15 ניסן, חוה"מ 16-20, שביעי של פסח 21 ניסן,
 *   יום השואה 27 ניסן (Friday -> Thursday 26, Sunday -> Monday 28),
 *   יום הזיכרון / יום העצמאות 4 / 5 אייר (5 on Friday -> 3 / 4, on Shabbat -> 2 / 3,
 *   on Monday -> 5 / 6), ל"ג בעומר 18 אייר, יום ירושלים 28 אייר, שבועות 6 סיון,
 *   י"ז בתמוז 17 תמוז and תשעה באב 9 אב (Shabbat -> Sunday), ט"ו באב 15 אב,
 *   ראש חודש: day 1 (not תשרי) and day 30 of the month before.
 */

#include "holidays.h"
#include "hebrew_date.h"

#include <stdio.h>
#include <string.h>

enum { SUN, MON, TUE, WED, THU, FRI, SHABBAT };

static void add(char *buf, size_t len, const char *text) {
    if (buf[0]) strncat(buf, ", ", len - strlen(buf) - 1);
    strncat(buf, text, len - strlen(buf) - 1);
}

/** The month after `month` (for ראש חודש on day 30) */
static int next_month(int month, int year) {
    if (month == HM_ADAR_II) return HM_NISAN;
    if (month == HM_ADAR_I) return hd_is_leap(year) ? HM_ADAR_II : HM_NISAN;
    return month + 1;
}

bool holiday_events(int year, int month, int day, char *buf, size_t len) {
    hdate_t hd = hd_from_greg(year, month, day);
    long today = hd_day_number_greg(year, month, day);
    int dow = (int)(today % 7);
    int m = hd.month, d = hd.day, y = hd.year;
    bool leap = hd_is_leap(y);
    int adar = leap ? HM_ADAR_II : HM_ADAR_I;   // the month of Purim
    char tmp[48];

    buf[0] = '\0';

    switch (m) {
        case HM_TISHREI:
            if (d == 1 || d == 2) add(buf, len, "ראש השנה");
            if ((d == 3 && dow != SHABBAT) || (d == 4 && dow == SUN)) add(buf, len, "צום גדליה");
            if (d == 10) add(buf, len, "יום כיפור");
            if (d == 15) add(buf, len, "סוכות");
            if (d >= 16 && d <= 20) add(buf, len, "חוה\"מ סוכות");
            if (d == 21) add(buf, len, "הושענא רבה");
            if (d == 22) add(buf, len, "שמחת תורה");
            break;
        case HM_SHVAT:
            if (d == 15) add(buf, len, "ט\"ו בשבט");
            break;
        case HM_NISAN:
            if (d == 15) add(buf, len, "פסח");
            if (d >= 16 && d <= 20) add(buf, len, "חוה\"מ פסח");
            if (d == 21) add(buf, len, "שביעי של פסח");
            if ((d == 27 && dow != FRI && dow != SUN) || (d == 26 && dow == THU) || (d == 28 && dow == MON))
                add(buf, len, "יום השואה");
            break;
        case HM_IYYAR: {
            // Day of the week of 5 Iyar decides where Yom HaAtzmaut goes
            int dow5 = ((dow + (5 - d)) % 7 + 7) % 7;
            int atzmaut = dow5 == FRI ? 4 : dow5 == SHABBAT ? 3 : dow5 == MON ? 6 : 5;
            if (d == atzmaut - 1) add(buf, len, "יום הזיכרון");
            if (d == atzmaut) add(buf, len, "יום העצמאות");
            if (d == 18) add(buf, len, "ל\"ג בעומר");
            if (d == 28) add(buf, len, "יום ירושלים");
            break;
        }
        case HM_SIVAN:
            if (d == 6) add(buf, len, "שבועות");
            break;
        case HM_TAMUZ:
            if ((d == 17 && dow != SHABBAT) || (d == 18 && dow == SUN)) add(buf, len, "י\"ז בתמוז");
            break;
        case HM_AV:
            if ((d == 9 && dow != SHABBAT) || (d == 10 && dow == SUN)) add(buf, len, "תשעה באב");
            if (d == 15) add(buf, len, "ט\"ו באב");
            break;
        case HM_TEVET:
            if (d == 10) add(buf, len, "עשרה בטבת");
            break;
        default:
            break;
    }

    // Purim (Adar, or Adar II in a leap year) and Purim Katan (Adar I in a leap year)
    if (m == adar) {
        if ((d == 13 && dow != SHABBAT) || (d == 11 && dow == THU)) add(buf, len, "תענית אסתר");
        if (d == 14) add(buf, len, "פורים");
        if (d == 15) add(buf, len, "שושן פורים");
    }
    if (leap && m == HM_ADAR_I && d == 14) add(buf, len, "פורים קטן");

    // Chanukah: 8 days from 25 Kislev (into Tevet). Kislev and Tevet belong to the
    // same Hebrew year (the year number changes at Tishrei).
    long chanukah = today - hd_day_number_hebrew(y, HM_KISLEV, 25) + 1;
    if (chanukah >= 1 && chanukah <= 8) {
        char n[8];
        hd_format_number((int)chanukah, true, n, sizeof(n));
        snprintf(tmp, sizeof(tmp), "חנוכה %s", n);
        add(buf, len, tmp);
    }

    // Rosh Chodesh: day 1 (except Tishrei = Rosh Hashana) and day 30 of the month before.
    // Short form when it shares the day with another event.
    int rc_month = 0;
    if (d == 1 && m != HM_TISHREI) rc_month = m;
    if (d == 30) rc_month = next_month(m, y);
    if (rc_month) {
        if (buf[0]) {
            add(buf, len, "ר\"ח");
        } else {
            snprintf(tmp, sizeof(tmp), "ראש חודש %s", hd_month_name(rc_month, y));
            add(buf, len, tmp);
        }
    }

    return buf[0] != '\0';
}
