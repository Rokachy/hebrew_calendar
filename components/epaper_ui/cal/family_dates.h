/**
 * @file family_dates.h
 *
 * Personal family dates (birthdays, anniversaries, ...) by the Hebrew calendar.
 *
 * The list itself is private: it lives in family.h, which is NOT in git (both
 * repos are public). Copy family.example.h to family.h and fill it in.
 */

#ifndef FAMILY_DATES_H
#define FAMILY_DATES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include "hebrew_date.h"

/** Month for someone born in plain אדר: אדר in a normal year, אדר ב' in a leap year */
#define FAMILY_ADAR  100

typedef struct {
    int month;          /**< HM_NISAN ... HM_ADAR_II, or FAMILY_ADAR */
    int day;            /**< 1 ... 30 */
    const char *text;   /**< shown in the event column, e.g. "יום הולדת לדני" */
} family_date_t;

/**
 * True if a family date (month, day) falls on the Hebrew date `today`:
 *  - FAMILY_ADAR: אדר, or אדר ב' in a leap year
 *  - אדר א' / אדר ב' (born in a leap year): plain אדר in a normal year
 *  - day 30 in a month that has 29 days this year: the 1st of the next month
 */
bool family_date_matches(int month, int day, const hdate_t *today);

/**
 * Texts of all family dates on the given Gregorian date (month 1-12), joined
 * with ", ". @return false (and an empty string) if there are none
 */
bool family_events(int year, int month, int day, char *buf, size_t len);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*FAMILY_DATES_H*/
