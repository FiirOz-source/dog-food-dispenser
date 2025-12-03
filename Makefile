PROJECT_NAME := dog-food-dispenser
BUILD_DIR    := build

PORT         := /dev/cu.usbserial-0001

BOARD_FQBN   := esp8266:esp8266:generic
BAUD         := 115200
ARDUINO_CLI  := arduino-cli

all: compile

core:
	@echo "Mise à jour du core ESP32..."
	$(ARDUINO_CLI) core update-index
	$(ARDUINO_CLI) core install esp32:esp32

compile: core
	@echo "Compilation → $(BUILD_DIR)/"
	$(ARDUINO_CLI) compile --fqbn $(BOARD_FQBN) \
		--build-path "$(abspath $(BUILD_DIR))" \
		--warnings all \
		./

upload: compile
	@echo "Upload sur $(PORT)..."
	$(ARDUINO_CLI) upload -p "$(PORT)" --fqbn $(BOARD_FQBN) --input-dir "$(BUILD_DIR)" ./

run: upload
	$(ARDUINO_CLI) monitor -p "$(PORT)" --config baudrate=$(BAUD)

clean:
	rm -rf $(BUILD_DIR)

rebuild: clean compile

port:
	@echo "Port utilisé → $(PORT)"
	@arduino-cli board list | grep -B1 -A1 ESP32 || echo "Aucune carte détectée pour l'instant"

.PHONY: all core compile upload run clean rebuild port