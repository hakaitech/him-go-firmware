# **Project Himitsu: Component 1 \- ESP32 Firmware**

This document details the complete technical specification for the ESP32 Crypto-Gateway. This firmware is the **Root of Trust** for the entire system.

## **1\. Core Requirements**

* **Platform:** ESP-IDF (preferred for performance and TLS) or Arduino (for rapid prototyping).  
* **Hardware:** ESP32-WROOM-32E or ESP32-S3 (with PSRAM for larger buffers).  
* **Mode:** WIFI\_AP\_STA (Access Point \+ Station) running simultaneously.  
* **Storage:** NVS (Non-Volatile Storage) for all persistent data (keys, credentials).  
* **Cryptography:**  
  * **Identity:** 10-char Base58 hash (derived from esp\_random() bytes).  
  * **Keys:** ECC secp256r1 keypair (using mbedtls).  
  * **Encryption:** AES-256-GCM (using mbedtls).  
  * **Key Exchange:** ECDH (using mbedtls).  
  * **Hashing:** SHA-256 (using mbedtls).

## **2\. Program Flow & State Machine**

The firmware will operate in one of several states:

1. **STATE\_FRESH\_BOOT:** (First-time ever boot).  
   * Generates and saves a new Identity (hash \+ keypair) to NVS.  
   * Moves to STATE\_AWAITING\_CONFIG.  
2. **STATE\_AWAITING\_CONFIG:** (Has an identity, but no internet).  
   * Starts Wi-Fi AP.  
   * Starts DNS & Captive Portal.  
   * Waits for the user to submit upstream Wi-Fi credentials via the portal.  
   * On success, saves credentials to NVS and moves to STATE\_RUNNING.  
3. **STATE\_RUNNING:** (Normal operation).  
   * Starts Wi-Fi AP (for client app).  
   * Starts Wi-Fi STA and connects to the upstream Wi-Fi.  
   * **On STA Connect:**  
     * Registers its identity with the Central Server (/register).  
     * Connects to the MQTT broker and subscribes to its topic.  
   * Starts the Local Web Server (192.168.4.1) to handle app requests.  
4. **STATE\_DISPOSING:** (Triggered by /dispose\_identity).  
   * Calls /dispose on the Central Server.  
   * Securely erases NVS partition (nvs\_flash\_erase()).  
   * Restarts (esp\_restart()). Will enter STATE\_FRESH\_BOOT.

## **3\. Component Details**

### **3.1. Identity & Cryptography (himitsu\_crypto.h)**

This module will wrap mbedtls for all crypto operations.

* void generate\_identity(char\* out\_hash\_10, mbedtls\_pk\_context\* out\_keypair):  
  * Generates 8 random bytes (esp\_random()).  
  * Base58-encodes them to create the 10-char hash.  
  * Generates a new ECC secp256r1 keypair.  
  * Saves keypair to NVS (password-protected if possible).  
* void sign\_message(char\* message, char\* out\_signature): Uses the NVS private key to sign data.  
* bool verify\_signature(char\* message, char\* signature, mbedtls\_pk\_context\* pub\_key): Verifies a signature.  
* void encrypt\_aes\_gcm(char\* plaintext, char\* shared\_secret, char\* out\_ciphertext): Encrypts data.  
* void decrypt\_aes\_gcm(char\* ciphertext, char\* shared\_secret, char\* out\_plaintext): Decrypts data.  
* void generate\_shared\_secret(mbedtls\_pk\_context\* my\_key, mbedtls\_pk\_context\* their\_key, char\* out\_secret): Performs ECDH.

### **3.2. Wi-Fi & Captive Portal (himitsu\_wifi.h)**

* **AP Mode:**  
  * SSID: himitsu\_node\_\[last\_4\_of\_hash\]  
  * IP: Static 192.168.4.1  
  * DHCP Server: Enabled.  
* **Captive Portal:**  
  * A DNSServer that routes all requests to 192.168.4.1.  
  * Serves a simple HTML page (/) with Wi-Fi scanning (/scan-wifi) and a form to submit (/connect).  
  * /connect (POST): Receives SSID/Password, saves to NVS, and transitions state.  
* **STA Mode:**  
  * Reads credentials from NVS.  
  * Manages connection and re-connection logic.

### **3.3. Central Server Client (himitsu\_client.h)**

This module handles all *outgoing* communication.

* **HTTP Client:**  
  * Uses esp\_http\_client (with TLS support).  
  * register\_identity(hash, pub\_key\_pem): Calls POST /register.  
  * dispose\_identity(hash, signature): Calls POST /dispose.  
  * get\_pub\_key(hash, out\_pub\_key\_pem): Calls GET /get\_pub\_key.  
* **MQTT Client:**  
  * Uses esp\_mqtt\_client (with MQTTS support).  
  * Connects and subscribes to himitsu/messages/\[my\_hash\].  
  * on\_mqtt\_data(topic, data): Callback when a new message arrives. Stores it in a rx\_message\_queue (a QueueHandle\_t).  
  * publish\_message(topic, ciphertext): Publishes an encrypted message.

### **3.4. Local API Server (himitsu\_api.h)**

This is the core of the firmware, handling all *incoming* requests from the client app.

* **Tech:** ESPAsyncWebServer (preferred) or esp\_http\_server.  
* **CORS:** Must be enabled (\*) to allow the app to connect.  
* **Endpoints:**  
  * GET /get\_my\_identity  
    * **Response (200):** {'hash': '\[current\_10\_char\_hash\]'}  
  * POST /encrypt\_and\_send  
    * **Body:** {'recipient\_hash': '...', 'plaintext': '...'}  
    * **Action:**  
      1. Get recipient\_hash and plaintext from body.  
      2. Call himitsu\_client.get\_pub\_key(recipient\_hash). (Handle 404 error).  
      3. Load recipient's public key PEM into an mbedtls\_pk\_context.  
      4. Call himitsu\_crypto.generate\_shared\_secret() to get a shared secret.  
      5. Call himitsu\_crypto.encrypt\_aes\_gcm() to encrypt the plaintext.  
      6. Call himitsu\_client.publish\_message(recipient\_topic, ciphertext).  
      7. Return the ciphertext to the app.  
    * **Response (200):** {'ciphertext': '...'}  
    * **Response (404):** {'error': 'Recipient hash not found'}  
  * GET /fetch\_messages  
    * **Action:**  
      1. Check the rx\_message\_queue.  
      2. Dequeue all pending messages (up to a limit, e.g., 10).  
      3. Format them into a JSON array.  
    * **Response (200):** {'messages': \['ciphertext1', 'ciphertext2'\]} (Empty array if no messages).  
  * POST /decrypt  
    * **Body:** {'ciphertext': '...'}  
    * **Action:**  
      1. This is a challenge. The ciphertext was encrypted with a *shared secret*, not just our private key.  
      2. **Modification Required:** The ciphertext must also contain the *sender's hash*.  
      3. **Revised Flow:**  
         * GET /fetch\_messages returns {'messages': \[{'sender\_hash': '...', 'ciphertext': '...'}\]}.  
         * POST /decrypt body is {'sender\_hash': '...', 'ciphertext': '...'}.  
      4. **New /decrypt Action:**  
         1. Get sender\_hash and ciphertext from body.  
         2. Call himitsu\_client.get\_pub\_key(sender\_hash).  
         3. Perform ECDH to get the same shared secret.  
         4. Call himitsu\_crypto.decrypt\_aes\_gcm() to get the plaintext.  
    * **Response (200):** {'plaintext': '...'}  
  * POST /dispose\_identity  
    * **Action:**  
      1. Triggers STATE\_DISPOSING.  
      2. (Firmware starts background task to dispose and reboot).  
    * **Response (202):** {'status': 'Disposing...'} (The connection will be cut before the client sees it, which is fine).
