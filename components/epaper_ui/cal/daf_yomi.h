/**
 * @file daf_yomi.h
 *
 * דף יומי (Babylonian Talmud), calculated from the date: a fixed cycle through
 * the whole Shas, one daf a day. Same algorithm as Hebcal.
 */

#ifndef DAF_YOMI_H
#define DAF_YOMI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
 * Daf of the given Gregorian date (month 1-12).
 * @param tractate  Hebrew name, e.g. "בכורות"
 * @param daf       page number, e.g. 13
 * @return false before the first cycle (11 Sep 1923)
 */
bool daf_yomi(int year, int month, int day, const char **tractate, int *daf);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*DAF_YOMI_H*/
