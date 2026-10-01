#include "time_sync.h"

#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "esp_attr.h"
#include "esp_timer.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "nvs_flash.h"

#if __has_include("wifi_secrets.h")
    #include "wifi_secrets.h"
#else
    #error "Copy main/wifi_secrets.example.h to main/wifi_secrets.h and set your Wi-Fi name and password"
#endif

static const char *TAG = "TIME";

// Israel: UTC+2, summer time from the Friday before the last Sunday of March
// 02:00 to the last Sunday of October 02:00
#define TZ_ISRAEL         "IST-2IDT,M3.4.4/26,M10.5.0"
// Three servers (CONFIG_LWIP_SNTP_MAX_SERVERS=3): if one is down, another answers
#define NTP_SERVER_COUNT  3
#define NTP_SERVERS       { "pool.ntp.org", "time.google.com", "time.cloudflare.com" }
#define TIME_VALID_AFTER  1704067200   // 2024-01-01 00:00 UTC
#define WIFI_TIMEOUT_MS   20000
#define WIFI_MAX_RETRY    5
#define SNTP_TIMEOUT_MS   30000

// Kept in RTC memory: survives deep sleep (not a power loss)
static RTC_DATA_ATTR time_t last_sync;

// Statistics (see time_sync_measure_drift), also in RTC memory
static RTC_DATA_ATTR uint32_t ntp_tries;        // every attempt to get internet time
static RTC_DATA_ATTR uint32_t ntp_ok;           // attempts that got an answer
static RTC_DATA_ATTR bool drift_valid;
static RTC_DATA_ATTR int64_t drift_us;          // internet time - own clock, last measurement
static RTC_DATA_ATTR time_t drift_measured_at;  // own clock time of that measurement

// A measurement in progress: our own clock reading just before asking, and the
// steady microsecond timer at that moment (not affected by setting the clock)
static volatile bool measuring;
static int64_t own_clock_us_at_start, timer_us_at_start;
static SemaphoreHandle_t measured_sem;

static EventGroupHandle_t wifi_events;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
static int wifi_retries;

bool time_is_valid(void) {
    return time(NULL) > TIME_VALID_AFTER;
}

static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data) {
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        if (wifi_retries++ < WIFI_MAX_RETRY) {
            esp_wifi_connect();
        } else {
            xEventGroupSetBits(wifi_events, WIFI_FAIL_BIT);
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(wifi_events, WIFI_CONNECTED_BIT);
    }
}

static int64_t timeval_us(const struct timeval *tv) {
    return (int64_t)tv->tv_sec * 1000000 + tv->tv_usec;
}

/**
 * Called by ESP-IDF right after it has set the clock to the internet time `tv`.
 * When measuring: compare with what our own clock would show now, then put our
 * own time back, so the calendar keeps running on the ESP32's clock.
 */
static void ntp_received(struct timeval *tv) {
    if (!measuring) return;
    int64_t own_us = own_clock_us_at_start + (esp_timer_get_time() - timer_us_at_start);
    drift_us = timeval_us(tv) - own_us;
    struct timeval own = { .tv_sec = (time_t)(own_us / 1000000), .tv_usec = (suseconds_t)(own_us % 1000000) };
    settimeofday(&own, NULL);
    drift_measured_at = own.tv_sec;
    drift_valid = true;
    xSemaphoreGive(measured_sem);
}

/**
 * Connect and get the time over NTP, then switch Wi-Fi off again.
 * set_clock: true = set the clock from the internet; false = only measure the
 * difference to our own clock (drift statistics), the clock is left as it was.
 */
static bool sync_over_wifi(bool set_clock) {
    esp_err_t err = nvs_flash_init();   // Wi-Fi keeps its calibration in NVS
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_events = xEventGroupCreate();
    wifi_retries = 0;
    esp_event_handler_instance_t wifi_handler, ip_handler;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                        wifi_event_handler, NULL, &wifi_handler));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                        wifi_event_handler, NULL, &ip_handler));

    wifi_config_t wifi_config = { 0 };
    strncpy((char *)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, WIFI_PASSWORD, sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Connecting to Wi-Fi \"%s\"...", WIFI_SSID);
    EventBits_t bits = xEventGroupWaitBits(wifi_events, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE, pdFALSE, pdMS_TO_TICKS(WIFI_TIMEOUT_MS));
    bool synced = false;
    ntp_tries++;
    if (bits & WIFI_CONNECTED_BIT) {
        esp_sntp_config_t sntp_cfg = ESP_NETIF_SNTP_DEFAULT_CONFIG_MULTIPLE(NTP_SERVER_COUNT, NTP_SERVERS);
        sntp_cfg.sync_cb = ntp_received;
        if (!set_clock) {
            struct timeval now;
            measured_sem = xSemaphoreCreateBinary();
            gettimeofday(&now, NULL);
            own_clock_us_at_start = timeval_us(&now);
            timer_us_at_start = esp_timer_get_time();
            measuring = true;
        }
        esp_netif_sntp_init(&sntp_cfg);
        if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(SNTP_TIMEOUT_MS)) == ESP_OK) {
            ntp_ok++;
            synced = true;
            if (set_clock) {
                last_sync = time(NULL);
                drift_valid = false;            // a new start for the drift statistics
            } else {
                // ntp_received runs right after the wait is released: let it finish
                xSemaphoreTake(measured_sem, pdMS_TO_TICKS(1000));
            }
        } else {
            ESP_LOGW(TAG, "No answer from the time servers (NTP, UDP port 123 - blocked on this network?)");
        }
        esp_netif_sntp_deinit();
        measuring = false;
        if (measured_sem) {
            vSemaphoreDelete(measured_sem);
            measured_sem = NULL;
        }
    } else {
        ESP_LOGW(TAG, "Wi-Fi connection failed");
    }

    // Wi-Fi off: frees its RAM before LVGL builds the screen
    esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, ip_handler);
    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_handler);
    esp_wifi_stop();
    esp_wifi_deinit();
    esp_netif_destroy_default_wifi(netif);
    esp_event_loop_delete_default();
    vEventGroupDelete(wifi_events);
    return synced;
}

bool time_sync_ensure(void) {
    // The TZ setting is not kept across deep sleep, set it on every boot
    setenv("TZ", TZ_ISRAEL, 1);
    tzset();

    time_t now = time(NULL);
    bool due = !time_is_valid() || last_sync == 0 ||
               now - last_sync > (time_t)TIME_RESYNC_DAYS * 24 * 3600;
    if (!due) {
        ESP_LOGI(TAG, "RTC time is valid, no Wi-Fi needed");
    } else if (sync_over_wifi(true)) {
        ESP_LOGI(TAG, "Time synced over NTP");
    } else if (time_is_valid()) {
        ESP_LOGW(TAG, "Sync failed, keeping the RTC time");
    }

    if (time_is_valid()) {
        char buf[32];
        struct tm local;
        now = time(NULL);
        localtime_r(&now, &local);
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S %Z", &local);
        ESP_LOGI(TAG, "Local time: %s", buf);
        return true;
    }
    return false;
}

bool time_sync_measure_drift(void) {
    if (!time_is_valid() || last_sync == 0 || time(NULL) - last_sync < 60) {
        return false;   // the clock was just set: nothing to measure yet
    }
    if (!sync_over_wifi(false) || !drift_valid) {
        ESP_LOGW(TAG, "Drift: no internet time");
        return false;
    }
    int64_t hours_x10 = (int64_t)(drift_measured_at - last_sync) * 10 / 3600;
    int64_t ppm = hours_x10 > 0 ? drift_us * 10 / (hours_x10 * 3600) : 0;
    ESP_LOGI(TAG, "Drift: %+lld ms after %lld.%lld h since the clock was set (%+lld ppm, %+lld s per 30 days)",
             (long long)(drift_us / 1000), (long long)(hours_x10 / 10), (long long)(hours_x10 % 10),
             (long long)ppm, (long long)(ppm * 30 * 24 * 3600 / 1000000));
    return true;
}

void time_sync_get_stats(time_stats_t *st) {
    st->tries = ntp_tries;
    st->ok = ntp_ok;
    st->drift_valid = drift_valid;
    st->drift_ms = drift_valid ? (int32_t)(drift_us / 1000) : 0;
    st->hours_x10 = drift_valid ? (int32_t)((drift_measured_at - last_sync) * 10 / 3600) : 0;
}
