
TELESCOPE := nites

ifeq (${TELESCOPE}, nites)
    # Maximum close time of 10.5 seconds
    # This is 2 seconds more than the nominal 8.5s close
    # time to account for the shutter-close glitches
    MAX_SHUTTER_CLOSE_STEPS = 21
else
    $(error Unknown telescope ${TELESCOPE})
endif

DEVICE = atmega328p
F_CPU = 16000000UL

AVRDUDE = avrdude -c arduino -P /dev/tty.usbmodem* -p $(DEVICE)
OBJECTS = main.o usb.o softserial.o

#  -Wall -Wextra -Werror
COMPILE = avr-gcc -g -mmcu=$(DEVICE) -Os -std=gnu99 -funsigned-bitfields -fshort-enums \
                  -DF_CPU=$(F_CPU) -DMAX_SHUTTER_CLOSE_STEPS=21

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
	$(COMPILE) -c $< -o $@

main.elf: $(OBJECTS)
	$(COMPILE) -o main.elf $(OBJECTS) -Wl,-u,vfprintf -lprintf_flt -lm

main.hex: main.elf
	rm -f main.hex
	avr-objcopy -j .text -j .data -O ihex main.elf main.hex
