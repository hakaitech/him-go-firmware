# Build Instructions

## Current Status

The ESP32 firmware project structure has been created with all core components implemented according to FirmwareInstructions.md. However, due to network connectivity issues with PyPI during the build environment setup, some Python dependencies could not be installed automatically.

## Successfully Installed Components

- ESP-IDF v6.0 framework ✅
- Xtensa GCC toolchain for ESP32 ✅
- CMake build system ✅
- Core project files and source code ✅

## Python Packages Status

### Installed:
- construct
- psutil  
- tree-sitter
- tree-sitter-c
- esp-idf-kconfig
- esp-idf-nvs-partition-gen
- esp-idf-size
- esp-idf-diag
- freertos-gdb
- pyclang
- packaging
- click
- pyparsing
- cryptography
- rich

### Missing (due to network timeout):
- pyelftools
- esptool
- idf-component-manager
- esp-idf-monitor
- esp-coredump
- esp-idf-panic-decoder

## Manual Build Instructions

If you have access to a stable internet connection, you can complete the build setup:

### Option 1: Complete ESP-IDF Installation

```bash
# Navigate to ESP-IDF directory
cd /home/runner/esp-idf

# Re-run installation with better network
./install.sh esp32

# Source the environment
. ./export.sh

# Navigate to project
cd /home/runner/work/him-go-firmware/him-go-firmware

# Build the project
idf.py build
```

### Option 2: Install Missing Packages Manually

```bash
# Activate the ESP-IDF Python environment
source /home/runner/esp-idf/export.sh

# Install missing packages
pip install pyelftools esptool idf-component-manager esp-idf-monitor esp-coredump esp-idf-panic-decoder

# Navigate to project
cd /home/runner/work/him-go-firmware/him-go-firmware

# Build
idf.py build
```

### Option 3: Use Pre-installed System

On a system with ESP-IDF already properly installed:

```bash
# Clone the repository
git clone https://github.com/hakaitech/him-go-firmware.git
cd him-go-firmware

# Set target
idf.py set-target esp32

# Build
idf.py build

# Flash (adjust port as needed)
idf.py -p /dev/ttyUSB0 flash

# Monitor
idf.py -p /dev/ttyUSB0 monitor
```

## Build Output

Once the build completes successfully, you should see:

```
Project build complete. To flash, run:
 idf.py flash
```

The build artifacts will be in the `build/` directory:

- `himitsu_firmware.elf` - Executable and Linkable Format file
- `himitsu_firmware.bin` - Flashable binary
- `himitsu_firmware.map` - Memory map file
- `bootloader/bootloader.bin` - Bootloader binary
- `partition_table/partition-table.bin` - Partition table

## Flashing to Device

```bash
# Flash all (bootloader + partition table + app)
idf.py -p /dev/ttyUSB0 flash

# Or flash only the app (after first flash)
idf.py -p /dev/ttyUSB0 app-flash

# Monitor serial output
idf.py -p /dev/ttyUSB0 monitor
```

## Troubleshooting

### Python Dependency Issues

If you encounter missing Python packages:

```bash
# Check what's installed
pip list | grep esp-idf

# Install specific missing package
pip install <package-name>
```

### CMake Configuration Issues

If CMake fails to configure:

```bash
# Clean build directory
rm -rf build

# Reconfigure
idf.py reconfigure
```

### Network Timeout During Install

```bash
# Increase pip timeout
export PIP_DEFAULT_TIMEOUT=300

# Retry installation
pip install <package-name>
```

## Next Steps

After successful build and flash:

1. The device will boot into STATE_FRESH_BOOT
2. It will generate a new identity and save to NVS
3. It will transition to STATE_AWAITING_CONFIG
4. Connect to the WiFi AP: `himitsu_node_XXXX`
5. Navigate to http://192.168.4.1 to configure upstream WiFi
6. Once configured, device enters STATE_RUNNING
7. Access the API at http://192.168.4.1/get_my_identity

## Development

For development and debugging:

```bash
# Enable verbose build output
idf.py -v build

# View configuration menu
idf.py menuconfig

# Clean build
idf.py fullclean

# Monitor with custom baud rate
idf.py -p /dev/ttyUSB0 -b 921600 monitor
```

## Notes

- The firmware is built for ESP32 (xtensa architecture)
- Default partition table is "Single factory app, no OTA"
- WiFi credentials and identity are stored in NVS
- MBEDTLS is configured with hardware acceleration
- HTTP server runs on port 80
- MQTT client connects to configurable broker

## Contact

For issues or questions, refer to the main project repository at:
https://github.com/hakaitech/him-go-firmware
