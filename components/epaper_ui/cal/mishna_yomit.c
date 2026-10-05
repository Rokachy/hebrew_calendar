/**
 * @file mishna_yomit.c
 *
 * Two mishnayot a day through the whole Mishnah in the printed order, without
 * breaks: a day's pair can run across the end of a chapter or a tractate. 4192
 * mishnayot = 2096 days a cycle. The current cycle started on 25 Dec 2021 (OU);
 * checked: 5 Oct 2026 = אהלות ח, ג-ד. Chapter lengths from Sefaria.
 */

#include "mishna_yomit.h"
#include <stdio.h>
#include "hebrew_date.h"   // hd_format_number()
#include "sun_times.h"     // days_from_civil()

#define MAX_CHAPTERS 30    // כלים

typedef struct {
    const char *name;
    int chapters;
    unsigned char mishnayot[MAX_CHAPTERS];   // per chapter
} tractate_t;

static const tractate_t mishnah[] = {
    { "ברכות", 9, { 5, 8, 6, 7, 5, 8, 5, 8, 5 } },
    { "פאה", 8, { 6, 8, 8, 11, 8, 11, 8, 9 } },
    { "דמאי", 7, { 4, 5, 6, 7, 11, 12, 8 } },
    { "כלאים", 9, { 9, 11, 7, 9, 8, 9, 8, 6, 10 } },
    { "שביעית", 10, { 8, 10, 10, 10, 9, 6, 7, 11, 9, 9 } },
    { "תרומות", 11, { 10, 6, 9, 13, 9, 6, 7, 12, 7, 12, 10 } },
    { "מעשרות", 5, { 8, 8, 10, 6, 8 } },
    { "מעשר שני", 5, { 7, 10, 13, 12, 15 } },
    { "חלה", 4, { 9, 8, 10, 11 } },
    { "ערלה", 3, { 9, 17, 9 } },
    { "בכורים", 4, { 11, 11, 12, 5 } },
    { "שבת", 24, { 11, 7, 6, 2, 4, 10, 4, 7, 7, 6, 6, 6, 7, 4, 3, 8, 8, 3, 6, 5, 3, 6, 5, 5 } },
    { "עירובין", 10, { 10, 6, 9, 11, 9, 10, 11, 11, 4, 15 } },
    { "פסחים", 10, { 7, 8, 8, 9, 10, 6, 13, 8, 11, 9 } },
    { "שקלים", 8, { 7, 5, 4, 9, 6, 6, 7, 8 } },
    { "יומא", 8, { 8, 7, 11, 6, 7, 8, 5, 9 } },
    { "סוכה", 5, { 11, 9, 15, 10, 8 } },
    { "ביצה", 5, { 10, 10, 8, 7, 7 } },
    { "ראש השנה", 4, { 9, 9, 8, 9 } },
    { "תענית", 4, { 7, 10, 9, 8 } },
    { "מגילה", 4, { 11, 6, 6, 10 } },
    { "מועד קטן", 3, { 10, 5, 9 } },
    { "חגיגה", 3, { 8, 7, 8 } },
    { "יבמות", 16, { 4, 10, 10, 13, 6, 6, 6, 6, 6, 9, 7, 6, 13, 9, 10, 7 } },
    { "כתובות", 13, { 10, 10, 9, 12, 9, 7, 10, 8, 9, 6, 6, 4, 11 } },
    { "נדרים", 11, { 4, 5, 11, 8, 6, 10, 9, 7, 10, 8, 12 } },
    { "נזיר", 9, { 7, 10, 7, 7, 7, 11, 4, 2, 5 } },
    { "סוטה", 9, { 9, 6, 8, 5, 5, 4, 8, 7, 15 } },
    { "גיטין", 9, { 6, 7, 8, 9, 9, 7, 9, 10, 10 } },
    { "קידושין", 4, { 10, 10, 13, 14 } },
    { "בבא קמא", 10, { 4, 6, 11, 9, 7, 6, 7, 7, 12, 10 } },
    { "בבא מציעא", 10, { 8, 11, 12, 12, 11, 8, 11, 9, 13, 6 } },
    { "בבא בתרא", 10, { 6, 14, 8, 9, 11, 8, 4, 8, 10, 8 } },
    { "סנהדרין", 11, { 6, 5, 8, 5, 5, 6, 11, 7, 6, 6, 6 } },
    { "מכות", 3, { 10, 8, 16 } },
    { "שבועות", 8, { 7, 5, 11, 13, 5, 7, 8, 6 } },
    { "עדיות", 8, { 14, 10, 12, 12, 7, 3, 9, 7 } },
    { "עבודה זרה", 5, { 9, 7, 10, 12, 12 } },
    { "אבות", 6, { 18, 16, 18, 22, 23, 11 } },
    { "הוריות", 3, { 5, 7, 8 } },
    { "זבחים", 14, { 4, 5, 6, 6, 8, 7, 6, 12, 7, 8, 8, 6, 8, 10 } },
    { "מנחות", 13, { 4, 5, 7, 5, 9, 7, 6, 7, 9, 9, 9, 5, 11 } },
    { "חולין", 12, { 7, 10, 7, 7, 5, 7, 6, 6, 8, 4, 2, 5 } },
    { "בכורות", 9, { 7, 9, 4, 10, 6, 12, 7, 10, 8 } },
    { "ערכין", 9, { 4, 6, 5, 4, 6, 5, 5, 7, 8 } },
    { "תמורה", 7, { 6, 3, 5, 4, 6, 5, 6 } },
    { "כריתות", 6, { 7, 6, 10, 3, 8, 9 } },
    { "מעילה", 6, { 4, 9, 8, 6, 5, 6 } },
    { "תמיד", 7, { 4, 5, 9, 3, 6, 3, 4 } },
    { "מדות", 5, { 9, 6, 8, 7, 4 } },
    { "קינים", 3, { 4, 5, 6 } },
    { "כלים", 30, { 9, 8, 8, 4, 11, 4, 6, 11, 8, 8, 9, 8, 8, 8, 6, 8, 17, 9, 10, 7, 3, 10, 5, 17, 9, 9, 12, 10, 8, 4 } },
    { "אהלות", 18, { 8, 7, 7, 3, 7, 7, 6, 6, 16, 7, 9, 8, 6, 7, 10, 5, 5, 10 } },
    { "נגעים", 14, { 6, 5, 8, 11, 5, 8, 5, 10, 3, 10, 12, 7, 12, 13 } },
    { "פרה", 12, { 4, 5, 11, 4, 9, 5, 12, 11, 9, 6, 9, 11 } },
    { "טהרות", 10, { 9, 8, 8, 13, 9, 10, 9, 9, 9, 8 } },
    { "מקואות", 10, { 8, 10, 4, 5, 6, 11, 7, 5, 7, 8 } },
    { "נדה", 10, { 7, 7, 7, 7, 9, 14, 5, 4, 11, 8 } },
    { "מכשירין", 6, { 6, 11, 8, 10, 11, 8 } },
    { "זבים", 5, { 6, 4, 3, 7, 12 } },
    { "טבול יום", 4, { 5, 8, 6, 7 } },
    { "ידים", 4, { 5, 4, 5, 8 } },
    { "עוקצין", 3, { 6, 10, 12 } },
};
#define TRACTATE_COUNT ((int)(sizeof(mishnah) / sizeof(mishnah[0])))
#define CYCLE_DAYS     2096

/** The mishna at position `index` (0 = ברכות א, א) of the whole Mishnah */
static void mishna_at(long index, mishna_ref_t *ref) {
    for (int t = 0; t < TRACTATE_COUNT; t++) {
        for (int c = 0; c < mishnah[t].chapters; c++) {
            if (index < mishnah[t].mishnayot[c]) {
                ref->tractate = mishnah[t].name;
                ref->chapter = c + 1;
                ref->mishna = (int)index + 1;
                return;
            }
            index -= mishnah[t].mishnayot[c];
        }
    }
    *ref = (mishna_ref_t){ mishnah[0].name, 1, 1 };   // not reached: index < 4192
}

void mishna_yomit(int year, int month, int day, mishna_ref_t *first, mishna_ref_t *second) {
    long days = days_from_civil(year, month, day) - days_from_civil(2021, 12, 25);
    long day_in_cycle = ((days % CYCLE_DAYS) + CYCLE_DAYS) % CYCLE_DAYS;
    mishna_at(day_in_cycle * 2, first);
    mishna_at(day_in_cycle * 2 + 1, second);
}

void mishna_yomit_format(int year, int month, int day, char *buf, size_t len) {
    mishna_ref_t a, b;
    mishna_yomit(year, month, day, &a, &b);
    char ac[8], am[8], bc[8], bm[8];
    hd_format_number(a.chapter, false, ac, sizeof(ac));
    hd_format_number(a.mishna, false, am, sizeof(am));
    hd_format_number(b.chapter, false, bc, sizeof(bc));
    hd_format_number(b.mishna, false, bm, sizeof(bm));
    if (a.tractate != b.tractate) {
        snprintf(buf, len, "%s: %s, %s - %s: %s, %s", a.tractate, ac, am, b.tractate, bc, bm);
    } else if (a.chapter != b.chapter) {
        snprintf(buf, len, "%s: %s, %s - %s, %s", a.tractate, ac, am, bc, bm);
    } else {
        snprintf(buf, len, "%s: %s, %s-%s", a.tractate, ac, am, bm);
    }
}
