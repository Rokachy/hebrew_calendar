#include "user_data.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "family_dates.h"

static const char *TAG = "USER";

#define USER_PARTITION  "userdata"
#define USER_NAMESPACE  "user"
#define FAMILY_MAX      64     // keys fam0 ... fam63
#define FAMILY_TEXT_MAX 160    // bytes, UTF-8 (Hebrew: 2 per letter)

static char wifi_ssid[33];     // sizes of wifi_sta_config_t ssid / password
static char wifi_password[65];

static family_date_t family[FAMILY_MAX];
static size_t family_count;

static const struct {
    const char *name;
    int month;
} month_names[] = {
    { "nisan", HM_NISAN },     { "iyyar", HM_IYYAR },       { "sivan", HM_SIVAN },
    { "tamuz", HM_TAMUZ },     { "av", HM_AV },             { "elul", HM_ELUL },
    { "tishrei", HM_TISHREI }, { "cheshvan", HM_CHESHVAN }, { "kislev", HM_KISLEV },
    { "tevet", HM_TEVET },     { "shvat", HM_SHVAT },       { "adar", FAMILY_ADAR },
    { "adar1", HM_ADAR_I },    { "adar2", HM_ADAR_II },
};

static int month_from_name(const char *name) {
    for (size_t i = 0; i < sizeof(month_names) / sizeof(month_names[0]); i++) {
        if (strcasecmp(name, month_names[i].name) == 0) return month_names[i].month;
    }
    return 0;
}

/** "kislev 25 text" -> family[family_count] */
static void add_family_date(const char *key, const char *value) {
    char month_name[16];
    int day, text_at = 0;
    if (sscanf(value, "%15s %d %n", month_name, &day, &text_at) != 2 || text_at == 0 ||
        value[text_at] == '\0') {
        ESP_LOGW(TAG, "%s: expected \"month day text\"", key);
        return;
    }
    int month = month_from_name(month_name);
    if (month == 0 || day < 1 || day > 30) {
        ESP_LOGW(TAG, "%s: bad month or day \"%s %d\"", key, month_name, day);
        return;
    }
    char *text = strdup(value + text_at);   // kept until the next deep sleep
    if (text == NULL) return;
    family[family_count++] = (family_date_t){ month, day, text };
}

static void get_str(nvs_handle_t h, const char *key, char *buf, size_t len) {
    size_t size = len;
    esp_err_t err = nvs_get_str(h, key, buf, &size);
    if (err != ESP_OK) {
        buf[0] = '\0';
        if (err != ESP_ERR_NVS_NOT_FOUND) ESP_LOGW(TAG, "%s: %s", key, esp_err_to_name(err));
    }
}

bool user_data_load(void) {
    esp_err_t err = nvs_flash_init_partition(USER_PARTITION);
    nvs_handle_t h;
    if (err == ESP_OK) {
        err = nvs_open_from_partition(USER_PARTITION, USER_NAMESPACE, NVS_READONLY, &h);
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "No user data (%s): create user_data.csv and run idf.py flash",
                 esp_err_to_name(err));
        nvs_flash_deinit_partition(USER_PARTITION);
        return false;
    }

    get_str(h, "wifi_ssid", wifi_ssid, sizeof(wifi_ssid));
    get_str(h, "wifi_pass", wifi_password, sizeof(wifi_password));

    char key[8], value[FAMILY_TEXT_MAX];
    for (int i = 0; i < FAMILY_MAX; i++) {
        snprintf(key, sizeof(key), "fam%d", i);
        get_str(h, key, value, sizeof(value));
        if (value[0]) add_family_date(key, value);
    }
    family_dates_set(family, family_count);

    nvs_close(h);
    nvs_flash_deinit_partition(USER_PARTITION);
    ESP_LOGI(TAG, "Wi-Fi \"%s\", %u family dates", wifi_ssid, (unsigned)family_count);
    return wifi_ssid[0] != '\0' || family_count > 0;
}

const char *user_wifi_ssid(void) {
    return wifi_ssid;
}

const char *user_wifi_password(void) {
    return wifi_password;
}
