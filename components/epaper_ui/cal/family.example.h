/**
 * @file family.example.h
 *
 * Template for the private family dates. Copy this file to family.h (same
 * folder) and put in your own dates. family.h is in .gitignore in both repos,
 * so names and dates never reach GitHub; the sync script copies it to the HW.
 *
 * One line per date, by the Hebrew calendar: { month, day, "text" }.
 * Months: HM_TISHREI, HM_CHESHVAN, HM_KISLEV, HM_TEVET, HM_SHVAT,
 *         FAMILY_ADAR (born in plain אדר; אדר ב' in a leap year),
 *         HM_ADAR_I / HM_ADAR_II (born in a leap year),
 *         HM_NISAN, HM_IYYAR, HM_SIVAN, HM_TAMUZ, HM_AV, HM_ELUL.
 * Put the name first ("דני - יום הולדת"): text that does not fit is cut at
 * the end, so the name is what stays visible.
 */

#pragma once
#include "family_dates.h"

static const family_date_t family_dates[] = {
    { HM_KISLEV,   25, "דני - יום הולדת" },
    { FAMILY_ADAR,  7, "רות - יום הולדת" },
    { HM_SIVAN,    12, "דני ורות - נישואין" },
};
