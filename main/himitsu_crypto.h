#ifndef HIMITSU_CRYPTO_H
#define HIMITSU_CRYPTO_H

#include "esp_err.h"
#include "mbedtls/pk.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/ecdh.h"
#include "mbedtls/gcm.h"

/**
 * @brief Generate a new identity and keypair
 * 
 * @param out_hash Output buffer for 10-character identity hash (must be at least 11 bytes for null terminator)
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t generate_identity(char *out_hash);

/**
 * @brief Sign a message using the stored private key
 * 
 * @param message Message to sign
 * @param message_len Length of message
 * @param out_signature Output buffer for signature (base64 encoded)
 * @param out_signature_len Length of output buffer
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t sign_message(const char *message, size_t message_len, 
                       char *out_signature, size_t out_signature_len);

/**
 * @brief Verify a signature
 * 
 * @param message Message that was signed
 * @param message_len Length of message
 * @param signature Signature to verify (base64 encoded)
 * @param pub_key_pem Public key in PEM format
 * @return ESP_OK if signature is valid, ESP_FAIL otherwise
 */
esp_err_t verify_signature(const char *message, size_t message_len,
                           const char *signature, const char *pub_key_pem);

/**
 * @brief Encrypt data using AES-256-GCM
 * 
 * @param plaintext Plaintext to encrypt
 * @param plaintext_len Length of plaintext
 * @param shared_secret Shared secret for encryption (32 bytes)
 * @param out_ciphertext Output buffer for ciphertext (base64 encoded)
 * @param out_ciphertext_len Length of output buffer
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t encrypt_aes_gcm(const char *plaintext, size_t plaintext_len,
                         const uint8_t *shared_secret,
                         char *out_ciphertext, size_t out_ciphertext_len);

/**
 * @brief Decrypt data using AES-256-GCM
 * 
 * @param ciphertext Ciphertext to decrypt (base64 encoded)
 * @param shared_secret Shared secret for decryption (32 bytes)
 * @param out_plaintext Output buffer for plaintext
 * @param out_plaintext_len Length of output buffer
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t decrypt_aes_gcm(const char *ciphertext,
                         const uint8_t *shared_secret,
                         char *out_plaintext, size_t out_plaintext_len);

/**
 * @brief Generate shared secret using ECDH
 * 
 * @param their_pub_key_pem Their public key in PEM format
 * @param out_secret Output buffer for shared secret (32 bytes)
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t generate_shared_secret(const char *their_pub_key_pem,
                                 uint8_t *out_secret);

/**
 * @brief Get the public key in PEM format
 * 
 * @param out_pem Output buffer for PEM-encoded public key
 * @param out_pem_len Length of output buffer
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t get_public_key_pem(char *out_pem, size_t out_pem_len);

#endif // HIMITSU_CRYPTO_H
