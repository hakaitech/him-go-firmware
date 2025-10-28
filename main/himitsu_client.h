#ifndef HIMITSU_CLIENT_H
#define HIMITSU_CLIENT_H

#include "esp_err.h"

/**
 * @brief Register identity with central server
 * 
 * @param hash Identity hash
 * @param pub_key_pem Public key in PEM format
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t register_identity(const char *hash, const char *pub_key_pem);

/**
 * @brief Dispose identity on central server
 * 
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t dispose_identity(void);

/**
 * @brief Get public key for a given hash from central server
 * 
 * @param hash Identity hash to look up
 * @param out_pub_key_pem Output buffer for PEM-encoded public key
 * @param out_pub_key_pem_len Length of output buffer
 * @return ESP_OK on success, ESP_FAIL on error (including 404 not found)
 */
esp_err_t get_pub_key(const char *hash, char *out_pub_key_pem, size_t out_pub_key_pem_len);

/**
 * @brief Initialize MQTT client
 * 
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t mqtt_client_init(void);

/**
 * @brief Publish encrypted message via MQTT
 * 
 * @param recipient_hash Recipient's identity hash
 * @param ciphertext Encrypted message
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t mqtt_publish_message(const char *recipient_hash, const char *ciphertext);

/**
 * @brief Get the MQTT receive queue handle
 * 
 * @return Queue handle for received messages
 */
void* mqtt_get_rx_queue(void);

#endif // HIMITSU_CLIENT_H
