
TELESCOPE := w1m

ifeq (${TELESCOPE}, nites)
    # Maximum close time of 10.5 seconds
    # This is 2 seconds more than the nominal 8.5s close
    # time to account for the shutter-close glitches
    DEVICE = atmega328p
    F_CPU = 16000000UL
    AVRDUDE = avrdude -c arduino -P /dev/tty.usbmodem* -p $(DEVICE)
    OBJECTS = main.o usb_v1.o serial_v1.o
    CC_FLAGS = -DUSE_LUFA_CONFIG_HEADER -I. -ILUFA -DHARDWARE_VERSION_1 -DMAX_SHUTTER_CLOSE_STEPS=21
else
ifeq (${TELESCOPE}, w1m)
    DEVICE = atmega32u4
    F_CPU = 16000000UL
    AVRDUDE = avrdude -c avr109 -P /dev/tty.usbmodem* -p $(DEVICE) -b57600 -D -V
    OBJECTS = main.o usb_v2.o serial_v2.o \
        usb_v2_descriptors.o \
        LUFA/Drivers/USB/Class/Device/CDCClassDevice.o \
        LUFA/Drivers/USB/Core/AVR8/Endpoint_AVR8.o \
        LUFA/Drivers/USB/Core/AVR8/EndpointStream_AVR8.o \
        LUFA/Drivers/USB/Core/AVR8/USBController_AVR8.o \
        LUFA/Drivers/USB/Core/AVR8/USBInterrupt_AVR8.o \
        LUFA/Drivers/USB/Core/DeviceStandardReq.o \
        LUFA/Drivers/USB/Core/Events.o \
        LUFA/Drivers/USB/Core/USBTask.o
    CC_FLAGS = -DUSE_LUFA_CONFIG_HEADER -I. -ILUFA -DHARDWARE_VERSION_2 -DMAX_SHUTTER_CLOSE_STEPS=21 -DF_USB=$(F_CPU)
    MAX_SHUTTER_CLOSE_STEPS = 21
else
    $(error Unknown telescope ${TELESCOPE})
endif
endif

#  -Wall -Wextra -Werror
COMPILE = avr-gcc -g -mmcu=$(DEVICE) -Os -std=gnu99 -funsigned-bitfields -fshort-enums \
                  -DF_CPU=$(F_CPU)

all: main.hex

install: main.hex
	$(AVRDUDE) -U flash:w:main.hex:i

clean:
	rm -f reset main.hex main.elf $(OBJECTS)

disasm:	main.elf
	avr-objdump -d main.elf

size: main.elf
	avr-size -C --mcu=$(DEVICE) main.elf

debug: main.elf
	avarice -g --part $(DEVICE) --dragon --jtag usb --file main.elf :4242

.c.o:
	$(COMPILE) -c $< $(CC_FLAGS) -o $@

main.elf: $(OBJECTS)
	$(COMPILE) -o main.elf $(OBJECTS) -lm -Os

main.hex: main.elf
	rm -f main.hex
	avr-objcopy -j .text -j .data -O ihex main.elf main.hex
