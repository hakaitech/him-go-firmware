#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"

#include "himitsu_crypto.h"
#include "himitsu_wifi.h"
#include "himitsu_client.h"
#include "himitsu_api.h"

static const char *TAG = "HIMITSU_MAIN";

// State machine states
typedef enum {
    STATE_FRESH_BOOT,
    STATE_AWAITING_CONFIG,
    STATE_RUNNING,
    STATE_DISPOSING
} system_state_t;

static system_state_t current_state = STATE_FRESH_BOOT;

/**
 * @brief Check if identity exists in NVS
 */
static bool check_identity_exists(void) {
    nvs_handle_t nvs_handle;
    esp_err_t err;
    
    err = nvs_open("himitsu", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        return false;
    }
    
    size_t required_size = 0;
    err = nvs_get_str(nvs_handle, "identity_hash", NULL, &required_size);
    nvs_close(nvs_handle);
    
    return (err == ESP_OK && required_size > 0);
}

/**
 * @brief Check if WiFi credentials exist in NVS
 */
static bool check_wifi_credentials_exist(void) {
    nvs_handle_t nvs_handle;
    esp_err_t err;
    
    err = nvs_open("himitsu", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        return false;
    }
    
    size_t required_size = 0;
    err = nvs_get_str(nvs_handle, "wifi_ssid", NULL, &required_size);
    nvs_close(nvs_handle);
    
    return (err == ESP_OK && required_size > 0);
}

/**
 * @brief Handle STATE_FRESH_BOOT
 */
static void handle_fresh_boot(void) {
    ESP_LOGI(TAG, "STATE_FRESH_BOOT: Generating new identity");
    
    char identity_hash[11] = {0};
    
    // Generate identity and keypair
    if (generate_identity(identity_hash) == ESP_OK) {
        ESP_LOGI(TAG, "Generated identity: %s", identity_hash);
        current_state = STATE_AWAITING_CONFIG;
    } else {
        ESP_LOGE(TAG, "Failed to generate identity");
        vTaskDelay(pdMS_TO_TICKS(5000));
        esp_restart();
    }
}

/**
 * @brief Handle STATE_AWAITING_CONFIG
 */
static void handle_awaiting_config(void) {
    ESP_LOGI(TAG, "STATE_AWAITING_CONFIG: Starting captive portal");
    
    // Start WiFi AP
    wifi_init_ap();
    
    // Start captive portal server
    captive_portal_start();
    
    // Wait for configuration (this is handled by the captive portal callbacks)
    // Once configured, the callback will update the state to STATE_RUNNING
}

/**
 * @brief Handle STATE_RUNNING
 */
static void handle_running(void) {
    ESP_LOGI(TAG, "STATE_RUNNING: Starting normal operation");
    
    // Start WiFi AP for client apps
    wifi_init_ap();
    
    // Start WiFi STA and connect to upstream WiFi
    wifi_init_sta();
    
    // Wait for STA connection
    ESP_LOGI(TAG, "Waiting for WiFi connection...");
    
    // Once connected, register with central server and start MQTT
    // This will be handled by WiFi event handlers
    
    // Start local API server
    api_server_start();
    
    ESP_LOGI(TAG, "System running normally");
}

/**
 * @brief Handle STATE_DISPOSING
 */
static void handle_disposing(void) {
    ESP_LOGI(TAG, "STATE_DISPOSING: Disposing identity");
    
    // Call dispose on central server
    dispose_identity();
    
    // Erase NVS
    ESP_LOGI(TAG, "Erasing NVS...");
    nvs_flash_erase();
    
    // Restart
    ESP_LOGI(TAG, "Restarting...");
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
}

/**
 * @brief Main state machine task
 */
static void state_machine_task(void *pvParameters) {
    while (1) {
        switch (current_state) {
            case STATE_FRESH_BOOT:
                handle_fresh_boot();
                break;
                
            case STATE_AWAITING_CONFIG:
                handle_awaiting_config();
                // Stay in this state until configuration is received
                vTaskDelay(pdMS_TO_TICKS(1000));
                break;
                
            case STATE_RUNNING:
                handle_running();
                // Stay in this state indefinitely
                vTaskDelay(pdMS_TO_TICKS(10000));
                break;
                
            case STATE_DISPOSING:
                handle_disposing();
                // This will restart the device
                break;
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/**
 * @brief Set system state (called by other modules)
 */
void set_system_state(system_state_t new_state) {
    ESP_LOGI(TAG, "State transition: %d -> %d", current_state, new_state);
    current_state = new_state;
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== Himitsu Crypto-Gateway Starting ===");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize network interface
    ESP_ERROR_CHECK(esp_netif_init());
    
    // Initialize event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // Determine initial state
    if (!check_identity_exists()) {
        current_state = STATE_FRESH_BOOT;
    } else if (!check_wifi_credentials_exist()) {
        current_state = STATE_AWAITING_CONFIG;
    } else {
        current_state = STATE_RUNNING;
    }
    
    ESP_LOGI(TAG, "Initial state: %d", current_state);
    
    // Create state machine task
    xTaskCreate(state_machine_task, "state_machine", 8192, NULL, 5, NULL);
}
