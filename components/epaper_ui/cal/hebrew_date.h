/**
 * @file hebrew_date.h
 *
 * Gregorian -> Hebrew date conversion and Hebrew numerals.
 * Portable C, no LVGL: runs the same in the simulator and on the ESP32.
 */

#ifndef HEBREW_DATE_H
#define HEBREW_DATE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

/** Hebrew months, counted from Nisan (the year number changes at Tishrei) */
enum {
    HM_NISAN = 1, HM_IYYAR, HM_SIVAN, HM_TAMUZ, HM_AV, HM_ELUL,
    HM_TISHREI, HM_CHESHVAN, HM_KISLEV, HM_TEVET, HM_SHVAT,
    HM_ADAR_I,   /**< Adar in a normal year, Adar I in a leap year */
    HM_ADAR_II,  /**< only in a leap year */
};

typedef struct {
    int year;    /**< e.g. 5787 */
    int month;   /**< HM_NISAN ... HM_ADAR_II */
    int day;     /**< 1 ... 30 */
} hdate_t;

/** Hebrew date of a Gregorian date (month 1-12) */
hdate_t hd_from_greg(int year, int month, int day);

/** True if the Hebrew year has 13 months */
bool hd_is_leap(int year);

/** Month name: "תשרי", "חשוון", "אדר" / "אדר א'" / "אדר ב'" ... */
const char *hd_month_name(int month, int year);

/**
 * Number in Hebrew letters, e.g. 26 -> כ"ו, 15 -> ט"ו, 5787 -> תשפ"ז (thousands dropped).
 * With `punct` false the geresh / gershayim are left out (כו) for narrow columns.
 */
void hd_format_number(int n, bool punct, char *buf, size_t len);

/** Full date, e.g. כ"ו אלול תשפ"ו */
void hd_format_date(const hdate_t *hd, char *buf, size_t len);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*HEBREW_DATE_H*/
