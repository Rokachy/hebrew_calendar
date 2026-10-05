/**
 * @file mishna_yomit.h
 *
 * משנה יומית, calculated from the date: a fixed cycle through the whole Mishnah,
 * two mishnayot a day.
 */

#ifndef MISHNA_YOMIT_H
#define MISHNA_YOMIT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef struct {
    const char *tractate;   /**< Hebrew name, e.g. "אהלות" */
    int chapter;            /**< 1-based */
    int mishna;             /**< 1-based */
} mishna_ref_t;

/** The two mishnayot of the given Gregorian date (month 1-12) */
void mishna_yomit(int year, int month, int day, mishna_ref_t *first, mishna_ref_t *second);

/**
 * The same as display text:
 *   "אהלות: ח, ג-ד"              both in one chapter
 *   "אהלות: ח, ו - ט, א"         across a chapter end
 *   "אהלות: יח, י - נגעים: א, א" across a tractate end
 */
void mishna_yomit_format(int year, int month, int day, char *buf, size_t len);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*MISHNA_YOMIT_H*/
