# Himitsu ESP32 Crypto-Gateway Firmware

This is the ESP32 firmware for Project Himitsu, a crypto-gateway device based on ESP32-WROOM-32E or ESP32-S3.

## Overview

The firmware implements a secure cryptographic gateway with the following features:

- **Identity Management**: 10-character Base58 identity hash with ECC secp256r1 keypair
- **State Machine**: FRESH_BOOT → AWAITING_CONFIG → RUNNING → DISPOSING
- **WiFi Modes**: Dual AP+STA mode for captive portal and internet connectivity
- **Cryptography**: AES-256-GCM encryption, ECDH key exchange, SHA-256 hashing
- **Local API Server**: REST API for client applications
- **MQTT Client**: Secure message routing through central server

## Project Structure

```
.
├── CMakeLists.txt              # Top-level CMake configuration
├── sdkconfig.defaults          # Default ESP-IDF configuration
├── main/
│   ├── CMakeLists.txt          # Main component build configuration
│   ├── himitsu_main.c          # Main application and state machine
│   ├── himitsu_crypto.c/h      # Cryptography module (identity, ECC, AES-GCM)
│   ├── himitsu_wifi.c/h        # WiFi and captive portal
│   ├── himitsu_client.c/h      # HTTP/MQTT client for central server
│   └── himitsu_api.c/h         # Local REST API server
└── README.md                   # This file
```

## Components

### 1. Main Application (`himitsu_main.c`)

Implements the core state machine:

- **STATE_FRESH_BOOT**: Generate new identity and keypair on first boot
- **STATE_AWAITING_CONFIG**: Run captive portal to collect WiFi credentials
- **STATE_RUNNING**: Normal operation with all services active
- **STATE_DISPOSING**: Securely erase identity and restart

### 2. Cryptography Module (`himitsu_crypto.c/h`)

- Identity generation with Base58 encoding
- ECC secp256r1 keypair generation and storage
- Message signing and verification
- AES-256-GCM encryption/decryption
- ECDH shared secret generation
- NVS storage for keys and identity

### 3. WiFi Module (`himitsu_wifi.c/h`)

- WiFi AP mode: `himitsu_node_[last_4_of_hash]` at 192.168.4.1
- WiFi STA mode: Connect to upstream WiFi
- Captive portal for initial configuration
- WiFi scanning and JSON output

### 4. Central Server Client (`himitsu_client.c/h`)

- HTTP client for identity registration and disposal
- Public key lookup from central server
- MQTT client for message routing
- Receive queue for incoming messages

### 5. Local API Server (`himitsu_api.c/h`)

REST API endpoints:

- `GET /get_my_identity` - Returns device identity hash
- `POST /encrypt_and_send` - Encrypt and send message to recipient
- `GET /fetch_messages` - Retrieve pending messages
- `POST /decrypt` - Decrypt received message
- `POST /dispose_identity` - Trigger identity disposal

All endpoints support CORS for browser-based clients.

## Building

### Prerequisites

- ESP-IDF v6.0 or later
- Python 3.8+
- CMake 3.16+

### Setup

1. Install ESP-IDF:
   ```bash
   git clone --recursive https://github.com/espressif/esp-idf.git
   cd esp-idf
   ./install.sh esp32
   . ./export.sh
   ```

2. Clone this repository:
   ```bash
   git clone https://github.com/hakaitech/him-go-firmware.git
   cd him-go-firmware
   ```

### Build

```bash
# Configure for ESP32
idf.py set-target esp32

# Build the project
idf.py build

# Flash to device (adjust port as needed)
idf.py -p /dev/ttyUSB0 flash

# Monitor serial output
idf.py -p /dev/ttyUSB0 monitor
```

### Configuration

Modify `sdkconfig.defaults` or use `idf.py menuconfig` to adjust:

- WiFi buffer sizes
- LWIP socket configuration
- MBEDTLS cryptography options
- HTTP server parameters
- Partition table

## Hardware Requirements

- ESP32-WROOM-32E or ESP32-S3 (recommended with PSRAM)
- USB cable for flashing and debugging
- 5V power supply (for standalone operation)

## API Usage

### Get Device Identity

```bash
curl http://192.168.4.1/get_my_identity
```

Response:
```json
{
  "hash": "Ab3xY7zQ12"
}
```

### Encrypt and Send Message

```bash
curl -X POST http://192.168.4.1/encrypt_and_send \
  -H "Content-Type: application/json" \
  -d '{"recipient_hash":"Cd5wT9mN34","plaintext":"Hello World"}'
```

Response:
```json
{
  "ciphertext": "base64_encrypted_data..."
}
```

### Fetch Messages

```bash
curl http://192.168.4.1/fetch_messages
```

Response:
```json
{
  "messages": [
    {
      "sender_hash": "Ef7pL2qR56",
      "ciphertext": "base64_encrypted_data..."
    }
  ]
}
```

### Decrypt Message

```bash
curl -X POST http://192.168.4.1/decrypt \
  -H "Content-Type: application/json" \
  -d '{"sender_hash":"Ef7pL2qR56","ciphertext":"base64_encrypted_data..."}'
```

Response:
```json
{
  "plaintext": "Decrypted message content"
}
```

## Security Features

- **Hardware RNG**: Uses ESP32's hardware random number generator
- **Secure Storage**: Private keys stored in NVS (can be encrypted)
- **TLS Support**: HTTPS and MQTTS for all external communication
- **Perfect Forward Secrecy**: ECDH key exchange for each session
- **Secure Disposal**: Complete NVS erasure on identity disposal

## Development Status

The firmware structure and all core modules have been implemented with stub functions. The following components are functional:

✅ Project structure and build configuration
✅ State machine implementation
✅ Identity generation and cryptographic functions
✅ NVS storage for keys and configuration
✅ WiFi AP/STA mode initialization
✅ HTTP server with REST API endpoints
✅ MQTT client initialization

Still TODO:
- Complete captive portal HTML/JS interface
- Full ECDH implementation
- Complete AES-GCM decrypt function
- Signature verification
- HTTP client implementation for central server
- MQTT subscription and message handling
- Error handling and retry logic
- OTA update support

## License

[Specify license here]

## References

- [FirmwareInstructions.md](FirmwareInstructions.md) - Complete technical specification
- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/)
- [mbedtls Documentation](https://mbed-tls.readthedocs.io/)
