/**
 * @file hebrew_date.c
 *
 * The standard arithmetic Hebrew calendar (molad + postponement rules), as in
 * "Calendrical Calculations" and Hebcal. Days are counted as "absolute" days:
 * day 1 = 1 January of year 1 (proleptic Gregorian).
 */

#include "hebrew_date.h"

#include <stdio.h>
#include <string.h>

#define HEBREW_EPOCH  (-1373428L)   // absolute day before 1 Tishrei of year 1

bool hd_is_leap(int year) {
    return (1 + year * 7) % 19 < 7;
}

static int months_in_year(int year) {
    return hd_is_leap(year) ? 13 : 12;
}

/** Days from the Hebrew epoch to 1 Tishrei of `year` (molad + postponements) */
static long elapsed_days(int year) {
    long prev = year - 1;
    long m_elapsed = 235 * (prev / 19) + 12 * (prev % 19) + ((prev % 19) * 7 + 1) / 19;
    long p_elapsed = 204 + 793 * (m_elapsed % 1080);
    long h_elapsed = 5 + 12 * m_elapsed + 793 * (m_elapsed / 1080) + p_elapsed / 1080;
    long parts = (p_elapsed % 1080) + 1080 * (h_elapsed % 24);
    long day = 1 + 29 * m_elapsed + h_elapsed / 24;

    long alt_day = day;
    if (parts >= 19440 ||
        (day % 7 == 2 && parts >= 9924 && !hd_is_leap(year)) ||
        (day % 7 == 1 && parts >= 16789 && hd_is_leap(year - 1))) {
        alt_day++;
    }
    // Rosh Hashana never falls on Sunday, Wednesday or Friday
    if (alt_day % 7 == 0 || alt_day % 7 == 3 || alt_day % 7 == 5) {
        alt_day++;
    }
    return alt_day;
}

static int days_in_year(int year) {
    return (int)(elapsed_days(year + 1) - elapsed_days(year));
}

static int days_in_month(int month, int year) {
    switch (month) {
        case HM_IYYAR: case HM_TAMUZ: case HM_ELUL: case HM_TEVET: case HM_ADAR_II:
            return 29;
        case HM_ADAR_I:
            return hd_is_leap(year) ? 30 : 29;
        case HM_CHESHVAN:
            return days_in_year(year) % 10 == 5 ? 30 : 29;   // "full" year
        case HM_KISLEV:
            return days_in_year(year) % 10 == 3 ? 29 : 30;   // "deficient" year
        default:
            return 30;
    }
}

static long hebrew_to_abs(int year, int month, int day) {
    long days = day;
    if (month < HM_TISHREI) {
        for (int m = HM_TISHREI; m <= months_in_year(year); m++) days += days_in_month(m, year);
        for (int m = HM_NISAN; m < month; m++) days += days_in_month(m, year);
    } else {
        for (int m = HM_TISHREI; m < month; m++) days += days_in_month(m, year);
    }
    return HEBREW_EPOCH + elapsed_days(year) + days - 1;
}

static long greg_to_abs(int year, int month, int day) {
    static const int days_before[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
    bool leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
    long y = year - 1;
    long day_of_year = days_before[month - 1] + day + (leap && month > 2 ? 1 : 0);
    return day_of_year + 365 * y + y / 4 - y / 100 + y / 400;
}

hdate_t hd_from_greg(int year, int month, int day) {
    long abs_day = greg_to_abs(year, month, day);
    hdate_t hd;

    // Start just below the answer and step forward to the right year
    hd.year = (int)((abs_day - HEBREW_EPOCH) / 366);
    while (hebrew_to_abs(hd.year + 1, HM_TISHREI, 1) <= abs_day) hd.year++;

    hd.month = abs_day < hebrew_to_abs(hd.year, HM_NISAN, 1) ? HM_TISHREI : HM_NISAN;
    while (abs_day > hebrew_to_abs(hd.year, hd.month, days_in_month(hd.month, hd.year))) hd.month++;

    hd.day = (int)(abs_day - hebrew_to_abs(hd.year, hd.month, 1)) + 1;
    return hd;
}

const char *hd_month_name(int month, int year) {
    static const char * const names[] = {
        "", "ניסן", "אייר", "סיון", "תמוז", "אב", "אלול",
        "תשרי", "חשוון", "כסלו", "טבת", "שבט", "אדר", "אדר ב'",
    };
    if (month == HM_ADAR_I && hd_is_leap(year)) return "אדר א'";
    if (month < HM_NISAN || month > HM_ADAR_II) return "";
    return names[month];
}

void hd_format_number(int n, bool punct, char *buf, size_t len) {
    static const char * const hundreds[] = { "", "ק", "ר", "ש", "ת" };
    static const char * const tens[] = { "", "י", "כ", "ל", "מ", "נ", "ס", "ע", "פ", "צ" };
    static const char * const ones[] = { "", "א", "ב", "ג", "ד", "ה", "ו", "ז", "ח", "ט" };

    const char *letters[8];
    int count = 0;

    n %= 1000;                          // years are written without the thousands
    while (n >= 400) { letters[count++] = "ת"; n -= 400; }
    if (n >= 100) { letters[count++] = hundreds[n / 100]; n %= 100; }
    if (n == 15) {                      // ט"ו / ט"ז instead of letters of God's name
        letters[count++] = "ט"; letters[count++] = "ו"; n = 0;
    } else if (n == 16) {
        letters[count++] = "ט"; letters[count++] = "ז"; n = 0;
    }
    if (n >= 10) { letters[count++] = tens[n / 10]; n %= 10; }
    if (n > 0) letters[count++] = ones[n];

    buf[0] = '\0';
    for (int i = 0; i < count; i++) {
        if (punct && count > 1 && i == count - 1) strncat(buf, "\"", len - strlen(buf) - 1);  // gershayim
        strncat(buf, letters[i], len - strlen(buf) - 1);
    }
    if (punct && count == 1) strncat(buf, "'", len - strlen(buf) - 1);                         // geresh
}

void hd_format_date(const hdate_t *hd, char *buf, size_t len) {
    char day[16], year[16];
    hd_format_number(hd->day, true, day, sizeof(day));
    hd_format_number(hd->year, true, year, sizeof(year));
    snprintf(buf, len, "%s %s %s", day, hd_month_name(hd->month, hd->year), year);
}
