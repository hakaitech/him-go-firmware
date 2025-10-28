#include "himitsu_wifi.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs.h"
#include <string.h>

static const char *TAG = "HIMITSU_WIFI";

esp_err_t wifi_init_ap(void) {
    ESP_LOGI(TAG, "Initializing WiFi AP");
    
    // Initialize WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    // Get identity hash for SSID
    nvs_handle_t nvs_handle;
    char identity_hash[11] = "0000";
    if (nvs_open("himitsu", NVS_READONLY, &nvs_handle) == ESP_OK) {
        size_t len = sizeof(identity_hash);
        nvs_get_str(nvs_handle, "identity_hash", identity_hash, &len);
        nvs_close(nvs_handle);
    }
    
    // Create SSID: himitsu_node_[last_4_of_hash]
    char ssid[32];
    snprintf(ssid, sizeof(ssid), "himitsu_node_%s", 
             identity_hash + (strlen(identity_hash) > 4 ? strlen(identity_hash) - 4 : 0));
    
    wifi_config_t wifi_config = {
        .ap = {
            .ssid_len = strlen(ssid),
            .channel = 1,
            .max_connection = 4,
            .authmode = WIFI_AUTH_OPEN,
        },
    };
    strncpy((char *)wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid));
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "WiFi AP started with SSID: %s", ssid);
    
    return ESP_OK;
}

esp_err_t wifi_init_sta(void) {
    ESP_LOGI(TAG, "Initializing WiFi STA");
    
    // Load credentials from NVS
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("himitsu", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS");
        return ESP_FAIL;
    }
    
    char ssid[32] = {0};
    char password[64] = {0};
    size_t len;
    
    len = sizeof(ssid);
    err = nvs_get_str(nvs_handle, "wifi_ssid", ssid, &len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read WiFi SSID");
        nvs_close(nvs_handle);
        return ESP_FAIL;
    }
    
    len = sizeof(password);
    err = nvs_get_str(nvs_handle, "wifi_password", password, &len);
    nvs_close(nvs_handle);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read WiFi password");
        return ESP_FAIL;
    }
    
    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_connect());
    
    ESP_LOGI(TAG, "WiFi STA connecting to: %s", ssid);
    
    return ESP_OK;
}

esp_err_t captive_portal_start(void) {
    ESP_LOGI(TAG, "Starting captive portal (stub)");
    // This would start a web server with DNS captive portal
    return ESP_OK;
}

esp_err_t captive_portal_stop(void) {
    ESP_LOGI(TAG, "Stopping captive portal (stub)");
    return ESP_OK;
}

esp_err_t wifi_scan_json(char *out_json, size_t out_json_len) {
    ESP_LOGI(TAG, "WiFi scan (stub)");
    snprintf(out_json, out_json_len, "{\"networks\":[]}");
    return ESP_OK;
}
