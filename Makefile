# Makefile for Himitsu ESP32 Firmware
# This is a convenience wrapper around idf.py

.PHONY: all build clean flash monitor menuconfig help

# Default target
all: build

# Build the project
build:
	idf.py build

# Clean build artifacts
clean:
	idf.py fullclean

# Flash to device (use ESPPORT to specify port, e.g., make flash ESPPORT=/dev/ttyUSB0)
flash:
	idf.py -p $(ESPPORT) flash

# Flash only the app (faster for development)
app-flash:
	idf.py -p $(ESPPORT) app-flash

# Monitor serial output
monitor:
	idf.py -p $(ESPPORT) monitor

# Flash and monitor in one command
flash-monitor: flash monitor

# Open configuration menu
menuconfig:
	idf.py menuconfig

# Erase flash completely
erase:
	idf.py -p $(ESPPORT) erase-flash

# Show project size information
size:
	idf.py size

# Show detailed component sizes
size-components:
	idf.py size-components

# Show detailed file sizes
size-files:
	idf.py size-files

# Set target to ESP32
set-target-esp32:
	idf.py set-target esp32

# Set target to ESP32-S3
set-target-esp32s3:
	idf.py set-target esp32s3

# Reconfigure project
reconfigure:
	idf.py reconfigure

# Help
help:
	@echo "Himitsu ESP32 Firmware Build System"
	@echo ""
	@echo "Available targets:"
	@echo "  all              - Build the project (default)"
	@echo "  build            - Build the project"
	@echo "  clean            - Clean build artifacts"
	@echo "  flash            - Flash firmware to device (set ESPPORT=/dev/ttyUSB0)"
	@echo "  app-flash        - Flash only app (faster)"
	@echo "  monitor          - Monitor serial output"
	@echo "  flash-monitor    - Flash and monitor"
	@echo "  menuconfig       - Open configuration menu"
	@echo "  erase            - Erase flash completely"
	@echo "  size             - Show project size"
	@echo "  size-components  - Show component sizes"
	@echo "  size-files       - Show file sizes"
	@echo "  set-target-esp32 - Set target to ESP32"
	@echo "  set-target-esp32s3 - Set target to ESP32-S3"
	@echo "  reconfigure      - Reconfigure project"
	@echo "  help             - Show this help"
	@echo ""
	@echo "Examples:"
	@echo "  make build"
	@echo "  make flash ESPPORT=/dev/ttyUSB0"
	@echo "  make flash-monitor ESPPORT=/dev/ttyUSB0"
	@echo "  make menuconfig"
