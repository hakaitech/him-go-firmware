#ifndef HIMITSU_API_H
#define HIMITSU_API_H

#include "esp_err.h"

/**
 * @brief Start the local API server
 * 
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t api_server_start(void);

/**
 * @brief Stop the local API server
 * 
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t api_server_stop(void);

/**
 * @brief Set system state (used by API handlers)
 * 
 * @param new_state New system state
 */
void set_system_state(int new_state);

#endif // HIMITSU_API_H
