#include "himitsu_api.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include <string.h>

static const char *TAG = "HIMITSU_API";
static httpd_handle_t server = NULL;

// Handler for GET /get_my_identity
static esp_err_t get_my_identity_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "GET /get_my_identity");
    
    // Set CORS headers
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_type(req, "application/json");
    
    // Get identity from NVS
    char response[128];
    snprintf(response, sizeof(response), "{\"hash\":\"stub_identity\"}");
    
    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Handler for POST /encrypt_and_send
static esp_err_t encrypt_and_send_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "POST /encrypt_and_send");
    
    // Set CORS headers
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_type(req, "application/json");
    
    // Read request body
    char buf[512];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Failed to read request");
        return ESP_FAIL;
    }
    buf[ret] = '\0';
    
    ESP_LOGI(TAG, "Received: %s", buf);
    
    // Parse JSON, encrypt, and send (stub)
    char response[128];
    snprintf(response, sizeof(response), "{\"ciphertext\":\"encrypted_stub\"}");
    
    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Handler for GET /fetch_messages
static esp_err_t fetch_messages_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "GET /fetch_messages");
    
    // Set CORS headers
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_type(req, "application/json");
    
    // Get messages from queue (stub)
    char response[256];
    snprintf(response, sizeof(response), "{\"messages\":[]}");
    
    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Handler for POST /decrypt
static esp_err_t decrypt_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "POST /decrypt");
    
    // Set CORS headers
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_type(req, "application/json");
    
    // Read request body
    char buf[512];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Failed to read request");
        return ESP_FAIL;
    }
    buf[ret] = '\0';
    
    // Decrypt (stub)
    char response[128];
    snprintf(response, sizeof(response), "{\"plaintext\":\"decrypted_stub\"}");
    
    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Handler for POST /dispose_identity
static esp_err_t dispose_identity_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "POST /dispose_identity");
    
    // Set CORS headers
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_type(req, "application/json");
    
    // Trigger STATE_DISPOSING
    set_system_state(3); // STATE_DISPOSING = 3
    
    char response[128];
    snprintf(response, sizeof(response), "{\"status\":\"Disposing...\"}");
    
    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Handler for OPTIONS (CORS preflight)
static esp_err_t options_handler(httpd_req_t *req) {
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

esp_err_t api_server_start(void) {
    ESP_LOGI(TAG, "Starting API server");
    
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.ctrl_port = 32768;
    config.lru_purge_enable = true;
    
    if (httpd_start(&server, &config) == ESP_OK) {
        // Register URI handlers
        httpd_uri_t get_my_identity = {
            .uri = "/get_my_identity",
            .method = HTTP_GET,
            .handler = get_my_identity_handler,
        };
        httpd_register_uri_handler(server, &get_my_identity);
        
        httpd_uri_t encrypt_and_send = {
            .uri = "/encrypt_and_send",
            .method = HTTP_POST,
            .handler = encrypt_and_send_handler,
        };
        httpd_register_uri_handler(server, &encrypt_and_send);
        
        httpd_uri_t fetch_messages = {
            .uri = "/fetch_messages",
            .method = HTTP_GET,
            .handler = fetch_messages_handler,
        };
        httpd_register_uri_handler(server, &fetch_messages);
        
        httpd_uri_t decrypt = {
            .uri = "/decrypt",
            .method = HTTP_POST,
            .handler = decrypt_handler,
        };
        httpd_register_uri_handler(server, &decrypt);
        
        httpd_uri_t dispose = {
            .uri = "/dispose_identity",
            .method = HTTP_POST,
            .handler = dispose_identity_handler,
        };
        httpd_register_uri_handler(server, &dispose);
        
        // Register OPTIONS handler for CORS
        httpd_uri_t options = {
            .uri = "/*",
            .method = HTTP_OPTIONS,
            .handler = options_handler,
        };
        httpd_register_uri_handler(server, &options);
        
        ESP_LOGI(TAG, "API server started on port 80");
        return ESP_OK;
    }
    
    ESP_LOGE(TAG, "Failed to start API server");
    return ESP_FAIL;
}

esp_err_t api_server_stop(void) {
    ESP_LOGI(TAG, "Stopping API server");
    
    if (server) {
        httpd_stop(server);
        server = NULL;
    }
    
    return ESP_OK;
}
