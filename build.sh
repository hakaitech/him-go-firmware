#!/bin/bash
#
# Complete Build Script for Himitsu ESP32 Firmware
# This script installs missing dependencies and builds the firmware
#

set -e

echo "========================================="
echo "Himitsu ESP32 Firmware Build Script"
echo "========================================="
echo ""

# Check if ESP-IDF is available
if [ -z "$IDF_PATH" ]; then
    echo "❌ ESP-IDF environment not set!"
    echo "Please run: source /path/to/esp-idf/export.sh"
    exit 1
fi

echo "✓ ESP-IDF found at: $IDF_PATH"
echo ""

# Install missing Python dependencies
echo "Installing Python dependencies..."
echo "This may take a few minutes with a stable internet connection."
echo ""

pip install -r requirements.txt || {
    echo ""
    echo "⚠️  Some packages failed to install."
    echo "Trying individual installation..."
    echo ""
    
    # Try installing critical packages individually
    for pkg in pyelftools esptool idf-component-manager esp-idf-monitor esp-coredump esp-idf-panic-decoder; do
        echo "Installing $pkg..."
        pip install --retries 5 --timeout 60 $pkg || echo "  ⚠️  $pkg failed, continuing..."
    done
}

echo ""
echo "Checking installed packages..."
pip list | grep -E "(esp-idf|pyelftools|esptool|construct)"

echo ""
echo "========================================="
echo "Building Firmware"
echo "========================================="
echo ""

# Set target
echo "Setting target to ESP32..."
idf.py set-target esp32

# Build
echo "Building project..."
idf.py build

echo ""
echo "========================================="
echo "Build Complete!"
echo "========================================="
echo ""
echo "To flash the firmware:"
echo "  idf.py -p /dev/ttyUSB0 flash"
echo ""
echo "To flash and monitor:"
echo "  idf.py -p /dev/ttyUSB0 flash monitor"
echo ""
echo "Build artifacts are in the 'build/' directory:"
ls -lh build/*.bin build/*.elf 2>/dev/null || echo "  (Run after successful build)"
echo ""
