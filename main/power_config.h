#pragma once

/*
 * How the device is powered. Decides how often it wakes and uses Wi-Fi:
 *
 *  POWER_BATTERY  one screen update a day (00:01), internet time every
 *                 TIME_RESYNC_DAYS days - as little awake time as possible.
 *  POWER_MAINS    screen updates at sunset and at 00:01, internet time every day.
 */
#define POWER_BATTERY  0
#define POWER_MAINS    1

#define POWER_SOURCE   POWER_BATTERY
