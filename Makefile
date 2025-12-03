# Makefile parfait pour ton arborescence actuelle
PROJECT_NAME := dog-food-dispenser
BUILD_DIR    := build

# Change selon ton OS
PORT         ?= /dev/cu.usbserial-*   # macOS (le plus souvent)
# PORT       ?= /dev/ttyUSB0          # Linux
# PORT       ?= COM3                  # Windows

BOARD_FQBN   := esp32:esp32:esp32     # 99% des ESP32 Dev Module
# BOARD_FQBN := esp32:esp32:esp32s3:USBCDC=1   # si tu as un S3

BAUD         := 115200
ARDUINO_CLI  := arduino-cli

all: compile

# Première fois ou quand tu ajoutes une lib
core:
	@echo "Mise à jour du core ESP32..."
	$(ARDUINO_CLI) core update-index
	$(ARDUINO_CLI) core install esp32:esp32

# Compilation
compile: core
	@echo "Compilation $(PROJECT_NAME) → $(BUILD_DIR)/"
	$(ARDUINO_CLI) compile --fqbn $(BOARD_FQBN) \
		--build-path $(BUILD_DIR) \
		--warnings all \
		--build-cache-path $(BUILD_DIR)/cache \
		./

# Flash
upload: compile
	@echo "Upload sur $(PORT)..."
	$(ARDUINO_CLI) upload -p $(PORT) --fqbn $(BOARD_FQBN) ./

# Flash + moniteur (la commande que tu vas taper 50 fois par jour)
run: upload
	$(ARDUINO_CLI) monitor -p $(PORT) --baud $(BAUD)

# Nettoyage
clean:
	rm -rf $(BUILD_DIR)

rebuild: clean compile

.PHONY: all core compile upload run clean rebuild