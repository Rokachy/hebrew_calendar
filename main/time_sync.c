#include "time_sync.h"

#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "esp_attr.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
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

/** Connect, sync the time over NTP, then switch Wi-Fi off again */
static bool sync_over_wifi(void) {
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
    if (bits & WIFI_CONNECTED_BIT) {
        esp_sntp_config_t sntp_cfg = ESP_NETIF_SNTP_DEFAULT_CONFIG_MULTIPLE(NTP_SERVER_COUNT, NTP_SERVERS);
        esp_netif_sntp_init(&sntp_cfg);
        if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(SNTP_TIMEOUT_MS)) == ESP_OK) {
            last_sync = time(NULL);
            synced = true;
        } else {
            ESP_LOGW(TAG, "No answer from the time servers (NTP, UDP port 123 - blocked on this network?)");
        }
        esp_netif_sntp_deinit();
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
    } else if (sync_over_wifi()) {
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
