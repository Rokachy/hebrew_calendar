/**
 * @file sun_times.c
 *
 * NOAA solar calculator equations (the "General Solar Position" spreadsheet),
 * refined twice so the sun position is taken at the time of the event itself.
 */

#include "sun_times.h"

#include <math.h>

#define DEG2RAD(x)  ((x) * M_PI / 180.0)
#define RAD2DEG(x)  ((x) * 180.0 / M_PI)

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

long days_from_civil(int year, int month, int day) {
    // Howard Hinnant's algorithm
    year -= month <= 2;
    long era = (year >= 0 ? year : year - 399) / 400;
    long yoe = year - era * 400;
    long doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
    long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

/** Sun declination (radians) and equation of time (minutes) at Julian day `jd` */
static void sun_position(double jd, double *decl, double *eqtime) {
    double t = (jd - 2451545.0) / 36525.0;   // Julian centuries since J2000

    double l0 = fmod(280.46646 + t * (36000.76983 + t * 0.0003032), 360.0);
    double m = 357.52911 + t * (35999.05029 - 0.0001537 * t);
    double e = 0.016708634 - t * (0.000042037 + 0.0000001267 * t);
    double c = sin(DEG2RAD(m)) * (1.914602 - t * (0.004817 + 0.000014 * t)) +
               sin(DEG2RAD(2 * m)) * (0.019993 - 0.000101 * t) +
               sin(DEG2RAD(3 * m)) * 0.000289;
    double omega = 125.04 - 1934.136 * t;
    double lambda = l0 + c - 0.00569 - 0.00478 * sin(DEG2RAD(omega));
    double eps0 = 23.0 + (26.0 + (21.448 - t * (46.815 + t * (0.00059 - t * 0.001813))) / 60.0) / 60.0;
    double eps = eps0 + 0.00256 * cos(DEG2RAD(omega));

    *decl = asin(sin(DEG2RAD(eps)) * sin(DEG2RAD(lambda)));

    double y = tan(DEG2RAD(eps / 2));
    y *= y;
    double l0r = DEG2RAD(l0), mr = DEG2RAD(m);
    *eqtime = 4.0 * RAD2DEG(y * sin(2 * l0r) - 2 * e * sin(mr) +
                            4 * e * y * sin(mr) * cos(2 * l0r) -
                            0.5 * y * y * sin(4 * l0r) - 1.25 * e * e * sin(2 * mr));
}

bool sun_time(int year, int month, int day, double latitude, double longitude,
              double zenith, bool rising, time_t *out) {
    long days = days_from_civil(year, month, day);
    double jd0 = days + 2440587.5;          // Julian day at 00:00 UTC
    double minutes = 720.0;                 // first guess: noon UTC

    for (int pass = 0; pass < 3; pass++) {
        double decl, eqtime;
        sun_position(jd0 + minutes / 1440.0, &decl, &eqtime);

        double lat = DEG2RAD(latitude);
        double cos_ha = cos(DEG2RAD(zenith)) / (cos(lat) * cos(decl)) - tan(lat) * tan(decl);
        if (cos_ha < -1.0 || cos_ha > 1.0) return false;   // sun never gets there today
        double ha = RAD2DEG(acos(cos_ha));

        minutes = 720.0 - 4.0 * (longitude + (rising ? ha : -ha)) - eqtime;
    }

    *out = (time_t)(days * 86400L + (long)lround(minutes * 60.0));
    return true;
}
