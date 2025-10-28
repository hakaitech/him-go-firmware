#include "himitsu_crypto.h"
#include "esp_log.h"
#include "esp_random.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "mbedtls/base64.h"
#include "mbedtls/sha256.h"
#include <string.h>

static const char *TAG = "HIMITSU_CRYPTO";

// Base58 alphabet (Bitcoin-style, without 0, O, I, l)
static const char BASE58_ALPHABET[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

/**
 * @brief Encode bytes to Base58
 */
static void encode_base58(const uint8_t *data, size_t data_len, char *out, size_t out_len) {
    // Simplified Base58 encoding (for identity hash generation)
    // This is a basic implementation - for production, use a proper Base58 library
    
    for (size_t i = 0; i < data_len && i < out_len - 1; i++) {
        out[i] = BASE58_ALPHABET[data[i] % 58];
    }
    out[data_len < out_len ? data_len : out_len - 1] = '\0';
}

esp_err_t generate_identity(char *out_hash) {
    ESP_LOGI(TAG, "Generating new identity");
    
    // Generate 8 random bytes
    uint8_t random_bytes[8];
    esp_fill_random(random_bytes, sizeof(random_bytes));
    
    // Encode to Base58 (10 characters)
    encode_base58(random_bytes, 8, out_hash, 11);
    
    // Ensure exactly 10 characters
    out_hash[10] = '\0';
    
    ESP_LOGI(TAG, "Generated identity hash: %s", out_hash);
    
    // Generate ECC keypair
    mbedtls_pk_context pk;
    mbedtls_pk_init(&pk);
    
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    
    const char *pers = "himitsu_keygen";
    int ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                     (const unsigned char *)pers, strlen(pers));
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to seed DRBG: -0x%04x", -ret);
        goto cleanup;
    }
    
    ret = mbedtls_pk_setup(&pk, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY));
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to setup PK context: -0x%04x", -ret);
        goto cleanup;
    }
    
    ret = mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256R1,
                              mbedtls_pk_ec(pk),
                              mbedtls_ctr_drbg_random, &ctr_drbg);
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to generate key: -0x%04x", -ret);
        goto cleanup;
    }
    
    ESP_LOGI(TAG, "Generated ECC keypair");
    
    // Save to NVS
    nvs_handle_t nvs_handle;
    ret = nvs_open("himitsu", NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS");
        goto cleanup;
    }
    
    // Save identity hash
    ret = nvs_set_str(nvs_handle, "identity_hash", out_hash);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save identity hash");
        nvs_close(nvs_handle);
        goto cleanup;
    }
    
    // Export and save private key
    unsigned char privkey_buf[2048];
    size_t privkey_len = 0;
    ret = mbedtls_pk_write_key_pem(&pk, privkey_buf, sizeof(privkey_buf));
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to write private key: -0x%04x", -ret);
        nvs_close(nvs_handle);
        goto cleanup;
    }
    
    privkey_len = strlen((char *)privkey_buf);
    ret = nvs_set_blob(nvs_handle, "private_key", privkey_buf, privkey_len);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save private key");
        nvs_close(nvs_handle);
        goto cleanup;
    }
    
    // Export and save public key
    unsigned char pubkey_buf[2048];
    ret = mbedtls_pk_write_pubkey_pem(&pk, pubkey_buf, sizeof(pubkey_buf));
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to write public key: -0x%04x", -ret);
        nvs_close(nvs_handle);
        goto cleanup;
    }
    
    size_t pubkey_len = strlen((char *)pubkey_buf);
    ret = nvs_set_blob(nvs_handle, "public_key", pubkey_buf, pubkey_len);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save public key");
        nvs_close(nvs_handle);
        goto cleanup;
    }
    
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    
    ESP_LOGI(TAG, "Identity saved to NVS");
    
cleanup:
    mbedtls_pk_free(&pk);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    
    return (ret == 0 || ret == ESP_OK) ? ESP_OK : ESP_FAIL;
}

esp_err_t sign_message(const char *message, size_t message_len,
                       char *out_signature, size_t out_signature_len) {
    ESP_LOGI(TAG, "Signing message");
    
    // Load private key from NVS
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("himitsu", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS");
        return ESP_FAIL;
    }
    
    size_t privkey_len = 0;
    err = nvs_get_blob(nvs_handle, "private_key", NULL, &privkey_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get private key size");
        nvs_close(nvs_handle);
        return ESP_FAIL;
    }
    
    unsigned char *privkey_buf = malloc(privkey_len);
    if (!privkey_buf) {
        nvs_close(nvs_handle);
        return ESP_FAIL;
    }
    
    err = nvs_get_blob(nvs_handle, "private_key", privkey_buf, &privkey_len);
    nvs_close(nvs_handle);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read private key");
        free(privkey_buf);
        return ESP_FAIL;
    }
    
    // Parse private key
    mbedtls_pk_context pk;
    mbedtls_pk_init(&pk);
    
    int ret = mbedtls_pk_parse_key(&pk, privkey_buf, privkey_len, NULL, 0,
                                    NULL, NULL);
    free(privkey_buf);
    
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to parse private key: -0x%04x", -ret);
        mbedtls_pk_free(&pk);
        return ESP_FAIL;
    }
    
    // Hash the message
    unsigned char hash[32];
    mbedtls_sha256((unsigned char *)message, message_len, hash, 0);
    
    // Sign the hash
    unsigned char sig_buf[MBEDTLS_MPI_MAX_SIZE];
    size_t sig_len = 0;
    
    ret = mbedtls_pk_sign(&pk, MBEDTLS_MD_SHA256, hash, 32,
                          sig_buf, sizeof(sig_buf), &sig_len,
                          NULL, NULL);
    
    mbedtls_pk_free(&pk);
    
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to sign: -0x%04x", -ret);
        return ESP_FAIL;
    }
    
    // Base64 encode the signature
    size_t olen = 0;
    ret = mbedtls_base64_encode((unsigned char *)out_signature, out_signature_len,
                                 &olen, sig_buf, sig_len);
    
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to encode signature: -0x%04x", -ret);
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

esp_err_t verify_signature(const char *message, size_t message_len,
                           const char *signature, const char *pub_key_pem) {
    // Implementation for signature verification
    // This would parse the public key and verify the signature
    ESP_LOGI(TAG, "Verifying signature (stub)");
    return ESP_OK;
}

esp_err_t encrypt_aes_gcm(const char *plaintext, size_t plaintext_len,
                         const uint8_t *shared_secret,
                         char *out_ciphertext, size_t out_ciphertext_len) {
    ESP_LOGI(TAG, "Encrypting with AES-GCM");
    
    mbedtls_gcm_context gcm;
    mbedtls_gcm_init(&gcm);
    
    int ret = mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, shared_secret, 256);
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to set GCM key: -0x%04x", -ret);
        mbedtls_gcm_free(&gcm);
        return ESP_FAIL;
    }
    
    // Generate random IV
    unsigned char iv[12];
    esp_fill_random(iv, sizeof(iv));
    
    // Allocate buffer for ciphertext + IV + tag
    size_t output_len = plaintext_len + 12 + 16; // IV + tag
    unsigned char *output_buf = malloc(output_len);
    if (!output_buf) {
        mbedtls_gcm_free(&gcm);
        return ESP_FAIL;
    }
    
    // Copy IV to output
    memcpy(output_buf, iv, 12);
    
    // Encrypt
    unsigned char tag[16];
    ret = mbedtls_gcm_crypt_and_tag(&gcm, MBEDTLS_GCM_ENCRYPT, plaintext_len,
                                     iv, 12, NULL, 0,
                                     (unsigned char *)plaintext, output_buf + 12,
                                     16, tag);
    
    mbedtls_gcm_free(&gcm);
    
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to encrypt: -0x%04x", -ret);
        free(output_buf);
        return ESP_FAIL;
    }
    
    // Append tag
    memcpy(output_buf + 12 + plaintext_len, tag, 16);
    
    // Base64 encode
    size_t olen = 0;
    ret = mbedtls_base64_encode((unsigned char *)out_ciphertext, out_ciphertext_len,
                                 &olen, output_buf, output_len);
    
    free(output_buf);
    
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to encode ciphertext: -0x%04x", -ret);
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

esp_err_t decrypt_aes_gcm(const char *ciphertext,
                         const uint8_t *shared_secret,
                         char *out_plaintext, size_t out_plaintext_len) {
    ESP_LOGI(TAG, "Decrypting with AES-GCM (stub)");
    // Similar to encrypt but in reverse
    return ESP_OK;
}

esp_err_t generate_shared_secret(const char *their_pub_key_pem,
                                 uint8_t *out_secret) {
    ESP_LOGI(TAG, "Generating shared secret via ECDH (stub)");
    // This would perform ECDH key exchange
    // For now, just fill with some data
    esp_fill_random(out_secret, 32);
    return ESP_OK;
}

esp_err_t get_public_key_pem(char *out_pem, size_t out_pem_len) {
    ESP_LOGI(TAG, "Getting public key PEM");
    
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("himitsu", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS");
        return ESP_FAIL;
    }
    
    size_t pubkey_len = out_pem_len;
    err = nvs_get_blob(nvs_handle, "public_key", out_pem, &pubkey_len);
    nvs_close(nvs_handle);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read public key");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}
