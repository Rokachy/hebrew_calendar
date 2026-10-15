#pragma once

/*
 * Private data (Wi-Fi name and password, family dates) from the "userdata" NVS
 * partition. The image is built from user_data.csv (project root, not in git;
 * template user_data.example.csv) and written by `idf.py flash`.
 */

#include <stdbool.h>

/** Read the partition and hand the family dates to the UI. False if it is missing or empty */
bool user_data_load(void);

/** Wi-Fi network name and password; "" if not set */
const char *user_wifi_ssid(void);
const char *user_wifi_password(void);
