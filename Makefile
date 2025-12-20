PROJECT_NAME := dog-food-dispenser
BUILD_DIR    := build

# Port → à modifier selon OS
# Mac/Linux : /dev/cu.usbserial-0001
# Windows : COM3, COM4, etc.
PORT         := /dev/cu.usbserial-0001

BOARD_FQBN   := esp8266:esp8266:generic
BAUD         := 115200
ARDUINO_CLI  := arduino-cli

LIB_DIR      := $(abspath libraries)

OS := $(shell uname 2>/dev/null || echo Windows)

all: compile

core:
	@echo "Mise à jour du core ESP8266..."
	$(ARDUINO_CLI) core update-index
	$(ARDUINO_CLI) core install esp8266:esp8266

compile: core
	@echo "Compilation → $(BUILD_DIR)/"
	$(ARDUINO_CLI) compile --fqbn $(BOARD_FQBN) \
		--build-path "$(abspath $(BUILD_DIR))" \
		--libraries "$(LIB_DIR)" \
		--warnings all \
		./

upload: compile
	@echo "Upload sur $(PORT)..."
	$(ARDUINO_CLI) upload -p "$(PORT)" --fqbn $(BOARD_FQBN) --input-dir "$(BUILD_DIR)" ./

run: upload
	$(ARDUINO_CLI) monitor -p "$(PORT)" --config baudrate=$(BAUD)

clean:
ifeq ($(OS),Windows)
	rmdir /s /q $(BUILD_DIR)
else
	rm -rf $(BUILD_DIR)
endif

rebuild: clean compile

port:
	@echo "Port utilisé → $(PORT)"
	@$(ARDUINO_CLI) board list || echo "Aucune carte détectée pour l'instant"

.PHONY: all core compile upload run clean rebuild port
