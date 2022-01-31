#PORT := "/dev/ttyUSB4"
PORT := "/dev/ttyACM0"

# Important: this must be set to the FBQN for the type
# of Arduino board you're using.  You can find this
# by searching with arduino-cli:
#   arduino-cli board search [search term]

# Adafruit Feather Huzzah
#BOARD := esp8266:esp8266:huzzah

# Adafruit Mag Tag
BOARD := esp32:esp32:adafruit_magtag29_esp32s2

# Arduino build flags
#AFLAGS := --build-property "build.extra_flags=\"-DCORE_DEBUG_LEVEL=ARDUHAL_LOG_LEVEL_DEBUG\""

SRC = $(wildcard *.ino)
SRC += $(wildcard *.cpp)
DEP = $(wildcard *.h)

DIRNAME:=$(shell basename $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST)))))
TARGET=$(DIRNAME).$(subst :,.,$(BOARD))

all: $(TARGET).elf

$(TARGET).elf: $(TARGET).bin

$(TARGET).bin: $(SRC) $(DEP)
	arduino-cli compile --fqbn $(BOARD) $(AFLAGS)

.PHONY: upload
upload: $(TARGET).elf
	test -e $(PORT) || { echo "Port not found!"; exit 1; }
	arduino-cli upload  --fqbn $(BOARD) -p $(PORT)

.PHONY: clean
clean:
	rm -f $(TARGET).elf $(TARGET).bin
