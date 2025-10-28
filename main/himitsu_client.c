#include "himitsu_client.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "mqtt_client.h"
#include "freertos/queue.h"
#include "nvs.h"

static const char *TAG = "HIMITSU_CLIENT";

// Configuration - these should be configurable
#define CENTRAL_SERVER_URL "https://api.himitsu.example.com"
#define MQTT_BROKER_URL "mqtts://mqtt.himitsu.example.com"

static QueueHandle_t rx_message_queue = NULL;
static esp_mqtt_client_handle_t mqtt_client = NULL;

esp_err_t register_identity(const char *hash, const char *pub_key_pem) {
    ESP_LOGI(TAG, "Registering identity: %s", hash);
    
    esp_http_client_config_t config = {
        .url = CENTRAL_SERVER_URL "/register",
        .method = HTTP_METHOD_POST,
        .timeout_ms = 5000,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_FAIL;
    }
    
    // Prepare JSON body
    char *json_data = malloc(strlen(hash) + strlen(pub_key_pem) + 100);
    if (!json_data) {
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }
    
    snprintf(json_data, strlen(hash) + strlen(pub_key_pem) + 100,
             "{\"hash\":\"%s\",\"pub_key\":\"%s\"}", hash, pub_key_pem);
    
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_data, strlen(json_data));
    
    esp_err_t err = esp_http_client_perform(client);
    int status_code = esp_http_client_get_status_code(client);
    
    free(json_data);
    esp_http_client_cleanup(client);
    
    if (err != ESP_OK || status_code != 200) {
        ESP_LOGE(TAG, "Registration failed: %s, status: %d", esp_err_to_name(err), status_code);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Identity registered successfully");
    return ESP_OK;
}

esp_err_t dispose_identity(void) {
    ESP_LOGI(TAG, "Disposing identity");
    
    // Get our identity hash
    nvs_handle_t nvs_handle;
    char identity_hash[11] = {0};
    
    if (nvs_open("himitsu", NVS_READONLY, &nvs_handle) == ESP_OK) {
        size_t len = sizeof(identity_hash);
        nvs_get_str(nvs_handle, "identity_hash", identity_hash, &len);
        nvs_close(nvs_handle);
    }
    
    esp_http_client_config_t config = {
        .url = CENTRAL_SERVER_URL "/dispose",
        .method = HTTP_METHOD_POST,
        .timeout_ms = 5000,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_FAIL;
    }
    
    // Prepare JSON body with identity hash
    char json_data[128];
    snprintf(json_data, sizeof(json_data), "{\"hash\":\"%s\"}", identity_hash);
    
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_data, strlen(json_data));
    
    esp_err_t err = esp_http_client_perform(client);
    int status_code = esp_http_client_get_status_code(client);
    
    esp_http_client_cleanup(client);
    
    if (err != ESP_OK || status_code != 200) {
        ESP_LOGE(TAG, "Disposal failed: %s, status: %d", esp_err_to_name(err), status_code);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Identity disposed on server");
    return ESP_OK;
}

esp_err_t get_pub_key(const char *hash, char *out_pub_key_pem, size_t out_pub_key_pem_len) {
    ESP_LOGI(TAG, "Getting public key for: %s", hash);
    
    char url[256];
    snprintf(url, sizeof(url), CENTRAL_SERVER_URL "/get_pub_key?hash=%s", hash);
    
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 5000,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_FAIL;
    }
    
    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }
    
    int content_length = esp_http_client_fetch_headers(client);
    int status_code = esp_http_client_get_status_code(client);
    
    if (status_code == 404) {
        ESP_LOGE(TAG, "Public key not found for hash: %s", hash);
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }
    
    if (status_code != 200 || content_length <= 0) {
        ESP_LOGE(TAG, "Failed to get public key, status: %d", status_code);
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }
    
    int read_len = esp_http_client_read(client, out_pub_key_pem, 
                                         out_pub_key_pem_len - 1);
    esp_http_client_cleanup(client);
    
    if (read_len > 0) {
        out_pub_key_pem[read_len] = '\0';
        ESP_LOGI(TAG, "Retrieved public key successfully");
        return ESP_OK;
    }
    
    ESP_LOGE(TAG, "Failed to read public key data");
    return ESP_FAIL;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, 
                               int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected");
            // Subscribe to our topic
            // esp_mqtt_client_subscribe(client, "himitsu/messages/[hash]", 0);
            break;
            
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT message received");
            // Add to rx queue
            if (rx_message_queue) {
                // In a real implementation, we'd parse the message and add to queue
                // xQueueSend(rx_message_queue, &message, 0);
            }
            break;
            
        default:
            break;
    }
}

esp_err_t mqtt_client_init(void) {
    ESP_LOGI(TAG, "Initializing MQTT client (stub)");
    
    // Create rx queue
    if (!rx_message_queue) {
        rx_message_queue = xQueueCreate(10, sizeof(void*));
    }
    
    // Initialize MQTT client
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URL,
    };
    
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
    
    return ESP_OK;
}

esp_err_t mqtt_publish_message(const char *recipient_hash, const char *ciphertext) {
    ESP_LOGI(TAG, "Publishing message to: %s (stub)", recipient_hash);
    
    // This would publish to topic: himitsu/messages/[recipient_hash]
    // with the ciphertext as payload
    
    if (mqtt_client) {
        char topic[128];
        snprintf(topic, sizeof(topic), "himitsu/messages/%s", recipient_hash);
        esp_mqtt_client_publish(mqtt_client, topic, ciphertext, 0, 1, 0);
    }
    
    return ESP_OK;
}

void* mqtt_get_rx_queue(void) {
    return rx_message_queue;
}
