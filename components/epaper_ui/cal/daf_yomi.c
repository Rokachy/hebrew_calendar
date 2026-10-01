/**
 * @file daf_yomi.c
 *
 * The cycle started on 11 Sep 1923. Cycles 1-7 took 2702 days (Shekalim with 13
 * dapim); from cycle 8 (24 Jun 1975) a cycle is 2711 days (Shekalim with 22,
 * Yerushalmi). Every tractate starts at daf 2, so a tractate of N dapim takes
 * N - 1 days.
 */

#include "daf_yomi.h"
#include "sun_times.h"   // days_from_civil()

typedef struct {
    const char *name;
    int dapim;           // last daf
} tractate_t;

static const tractate_t shas[] = {
    { "ברכות", 64 },      { "שבת", 157 },       { "עירובין", 105 },  { "פסחים", 121 },
    { "שקלים", 22 },      { "יומא", 88 },        { "סוכה", 56 },      { "ביצה", 40 },
    { "ראש השנה", 35 },   { "תענית", 31 },       { "מגילה", 32 },     { "מועד קטן", 29 },
    { "חגיגה", 27 },      { "יבמות", 122 },      { "כתובות", 112 },   { "נדרים", 91 },
    { "נזיר", 66 },       { "סוטה", 49 },        { "גיטין", 90 },     { "קידושין", 82 },
    { "בבא קמא", 119 },   { "בבא מציעא", 119 },  { "בבא בתרא", 176 }, { "סנהדרין", 113 },
    { "מכות", 24 },       { "שבועות", 49 },      { "עבודה זרה", 76 }, { "הוריות", 14 },
    { "זבחים", 120 },     { "מנחות", 110 },      { "חולין", 142 },    { "בכורות", 61 },
    { "ערכין", 34 },      { "תמורה", 34 },       { "כריתות", 28 },    { "מעילה", 22 },
    { "קינים", 4 },       { "תמיד", 9 },         { "מדות", 5 },       { "נדה", 73 },
};
#define SHAS_COUNT ((int)(sizeof(shas) / sizeof(shas[0])))
#define SHEKALIM   4

// Kinnim, Tamid and Midot are printed at the end of Meilah, so their dapim
// continue Meilah's numbering instead of starting at 2. Learned as (checked
// against Hebcal, March 2027): קינים 23-25, תמיד 26-33, מדות 34-37
static int daf_offset(int index) {
    switch (index) {
        case 36: return 21;   // קינים
        case 37: return 24;   // תמיד
        case 38: return 32;   // מדות
        default: return 0;
    }
}

bool daf_yomi(int year, int month, int day, const char **tractate, int *daf) {
    long today = days_from_civil(year, month, day);
    long first_cycle = days_from_civil(1923, 9, 11);
    long cycle8 = days_from_civil(1975, 6, 24);
    if (today < first_cycle) return false;

    bool old_cycle = today < cycle8;
    long day_in_cycle = old_cycle ? (today - first_cycle) % 2702 : (today - cycle8) % 2711;

    long total = 0;
    for (int i = 0; i < SHAS_COUNT; i++) {
        int dapim = (i == SHEKALIM && old_cycle) ? 13 : shas[i].dapim;
        total += dapim - 1;
        if (day_in_cycle < total) {
            *tractate = shas[i].name;
            *daf = (int)(dapim + 1 - (total - day_in_cycle)) + daf_offset(i);
            return true;
        }
    }
    return false;   // not reached: the cycle length is the sum of all tractates
}
