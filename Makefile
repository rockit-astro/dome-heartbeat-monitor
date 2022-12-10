# Dome Heartbeat Monitor v2
# Based on LUFA Library makefile template

# Run "make help" for target help.

# Maximum number of close steps to send to the dome
# Use 21 for the 7.5' old NITES dome
# Use 35 for the 12.5' new NITES dome
# Use 62 for the 20' W1m dome
# Use 62 for the 18' GOTO domes
MAX_SHUTTER_CLOSE_STEPS = 62

# Send the bumper guard reset command before closing?
# Use 1 for domes that have a bumper guard installed
# Use 0 otherwise
HAS_BUMPER_GUARD = 1

# Use 1 for the older boards that connect a siren to the ISCP header on the top of the Arduino
# Use 0 for the newer boards that have a siren header on the main board
EXTERNAL_SIREN = 0

# Use 0 to close the A side first
# Use 1 to close the B side first
CLOSE_B_FIRST = 1

MCU                = atmega32u4
ARCH               = AVR8
BOARD              = MICRO
F_CPU              = 16000000
F_USB              = $(F_CPU)
AVRDUDE_PROGRAMMER = avr109
AVRDUDE_PORT       = /dev/tty.usbmodem*

OPTIMIZATION = s
TARGET       = main
SRC          = main.c serial.c usb.c usb_descriptors.c $(LUFA_SRC_USB) $(LUFA_SRC_USBCLASS)
LUFA_PATH    = LUFA
CC_FLAGS     = -DUSE_LUFA_CONFIG_HEADER -DMAX_SHUTTER_CLOSE_STEPS=$(MAX_SHUTTER_CLOSE_STEPS) -DHAS_BUMPER_GUARD=$(HAS_BUMPER_GUARD) -DEXTERNAL_SIREN=$(EXTERNAL_SIREN) -DCLOSE_B_FIRST=$(CLOSE_B_FIRST)
LD_FLAGS     =

# Default target
all:

# Alias the avrdude target to install for convenience
install: avrdude

disasm:	main.elf
	avr-objdump -d main.elf

# Include LUFA-specific DMBS extension modules
DMBS_LUFA_PATH ?= $(LUFA_PATH)/Build/LUFA
include $(DMBS_LUFA_PATH)/lufa-sources.mk
include $(DMBS_LUFA_PATH)/lufa-gcc.mk

# Include common DMBS build system modules
DMBS_PATH      ?= $(LUFA_PATH)/Build/DMBS/DMBS
include $(DMBS_PATH)/core.mk
include $(DMBS_PATH)/cppcheck.mk
include $(DMBS_PATH)/doxygen.mk
include $(DMBS_PATH)/dfu.mk
include $(DMBS_PATH)/gcc.mk
include $(DMBS_PATH)/hid.mk
include $(DMBS_PATH)/avrdude.mk
include $(DMBS_PATH)/atprogram.mk
