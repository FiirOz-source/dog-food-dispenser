PROJECT_NAME := dog-food-dispenser
BUILD_DIR    := build

# Port → override possible:
#   make run-all PORT=/dev/cu.usbserial-A5069RR4
# Mac/Linux : /dev/cu.usbserial* /dev/cu.SLAB_USBtoUART* /dev/cu.wchusbserial*
# Linux     : /dev/ttyUSB* /dev/ttyACM*
# Windows   : COM3, COM4, etc.
PORT ?= $(shell ls /dev/cu.usbserial* /dev/cu.SLAB_USBtoUART* /dev/cu.wchusbserial* /dev/ttyUSB* /dev/ttyACM* 2>/dev/null | head -n 1)
ifeq ($(strip $(PORT)),)
  PORT := /dev/cu.usbserial-0001
endif

UPLOAD_BAUD  := 921600
BAUD         := 115200
ARDUINO_CLI  := arduino-cli

LIB_DIR      := $(abspath libraries)
DISP_LIB_DIR := $(abspath dispenser_lib)

OS := $(shell uname 2>/dev/null || echo Windows)

FLASH_SIZE   := 4M2M

BOARD_FQBN   := esp8266:esp8266:nodemcuv2:eesz=$(FLASH_SIZE),baud=$(UPLOAD_BAUD)

DATA_DIR := $(abspath data)
LFS_BIN  := $(abspath $(BUILD_DIR))/littlefs.bin

ifeq ($(FLASH_SIZE),4M2M)
  LFS_START := 0x200000
  LFS_SIZE  := 0x1FA000
else ifeq ($(FLASH_SIZE),4M1M)
  LFS_START := 0x300000
  LFS_SIZE  := 0x0FA000
else
  $(warning FlashSize=$(FLASH_SIZE) non géré pour LittleFS.)
  LFS_START := 0x200000
  LFS_SIZE  := 0x1FA000
endif

ARDUINO_DATA_DIR := $(shell $(ARDUINO_CLI) config get directories.data 2>/dev/null)

ifeq ($(strip $(ARDUINO_DATA_DIR)),)
  ifeq ($(OS),Darwin)
    ARDUINO_DATA_DIR := $(HOME)/Library/Arduino15
  else
    ARDUINO_DATA_DIR := $(HOME)/.arduino15
  endif
endif

ESP8266_TOOLS_DIR := $(ARDUINO_DATA_DIR)/packages/esp8266/tools

MKLITTLEFS := $(shell find "$(ESP8266_TOOLS_DIR)" -type f -name "mklittlefs*" 2>/dev/null | head -n 1)
UPLOAD_PY  := $(shell find "$(ARDUINO_DATA_DIR)/packages/esp8266/hardware" -type f -path "*/tools/upload.py" 2>/dev/null | sort | tail -n 1)

DOXYFILE ?= Doxyfile
DOC_DIR  ?= docs

.PHONY: docs docs-init docs-clean

docs-init:
	@command -v doxygen >/dev/null 2>&1 || { \
		echo "Erreur: doxygen n'est pas installé. -> brew install doxygen"; exit 127; \
	}
	@test -f "$(DOXYFILE)" || doxygen -g $(DOXYFILE)
	@echo "Assure-toi que ton Doxyfile contient au moins:"
	@echo "  GENERATE_HTML = YES"
	@echo "  OUTPUT_DIRECTORY = $(DOC_DIR)"
	@echo "  HTML_OUTPUT = html"

docs: docs-init
	@mkdir -p "$(DOC_DIR)"
	@doxygen "$(DOXYFILE)"
	@echo "Documentation générée."
	@echo "Ouvre: $(DOC_DIR)/html/index.html"
	@echo "macOS: open $(DOC_DIR)/html/index.html"

docs-clean:
	rm -rf "$(DOC_DIR)"

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
		--libraries "$(DISP_LIB_DIR)" \
		--warnings all \
		--build-property compiler.cpp.extra_flags=-fexceptions \
		--build-property compiler.c.extra_flags=-fexceptions \
		./

upload: compile
	@echo "Upload sketch sur $(PORT) (baud=$(UPLOAD_BAUD))..."
	$(ARDUINO_CLI) upload -p "$(PORT)" --fqbn $(BOARD_FQBN) --input-dir "$(BUILD_DIR)" ./

fs-image: core
	@test -d "$(DATA_DIR)" || (echo "Dossier data/ introuvable: $(DATA_DIR)"; exit 1)
	@test -n "$(MKLITTLEFS)" || (echo "mklittlefs introuvable dans $(ESP8266_TOOLS_DIR)"; exit 1)
	@mkdir -p "$(BUILD_DIR)"
	@echo "Build LittleFS -> $(LFS_BIN)"
	@echo "  eesz=$(FLASH_SIZE)  LFS_START=$(LFS_START)  LFS_SIZE=$(LFS_SIZE)"
	"$(MKLITTLEFS)" -c "$(DATA_DIR)" -p 256 -b 8192 -s $(LFS_SIZE) "$(LFS_BIN)"

fs-upload: fs-image
	@test -f "$(UPLOAD_PY)" || (echo "upload.py introuvable (core esp8266 installé ?)"; exit 1)
	@echo "Flash LittleFS -> start=$(LFS_START)"
	python3 "$(UPLOAD_PY)" --chip esp8266 --port "$(PORT)" --baud $(UPLOAD_BAUD) \
		write_flash $(LFS_START) "$(LFS_BIN)"

upload-all: upload fs-upload

run: upload
	$(ARDUINO_CLI) monitor -p "$(PORT)" --config baudrate=$(BAUD)

run-all: upload-all
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

.PHONY: all core compile upload upload-all run run-all clean rebuild port fs-image fs-upload