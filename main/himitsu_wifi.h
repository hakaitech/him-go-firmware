#ifndef HIMITSU_WIFI_H
#define HIMITSU_WIFI_H

#include "esp_err.h"

/**
 * @brief Initialize WiFi in AP mode
 * 
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t wifi_init_ap(void);

/**
 * @brief Initialize WiFi in STA mode and connect
 * 
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t wifi_init_sta(void);

/**
 * @brief Start captive portal
 * 
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t captive_portal_start(void);

/**
 * @brief Stop captive portal
 * 
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t captive_portal_stop(void);

/**
 * @brief Get WiFi scan results as JSON
 * 
 * @param out_json Output buffer for JSON string
 * @param out_json_len Length of output buffer
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t wifi_scan_json(char *out_json, size_t out_json_len);

#endif // HIMITSU_WIFI_H
