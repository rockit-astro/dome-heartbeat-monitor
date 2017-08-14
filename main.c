//**********************************************************************************
//  Copyright 2017 Paul Chote
//  This file is part of dome-heartbeat-monitor, which is free software. It is made
//  available to you under version 3 (or later) of the GNU General Public License,
//  as published by the Free Software Foundation and included in the LICENSE file.
//**********************************************************************************

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "usb.h"
#include "serial.h"

// TODO: Define LED pin equivalents for v1 hardware
#if defined(HARDWARE_VERSION_2)
    #define TRIGGER_ENABLE_PORT PORTC
    #define TRIGGER_ENABLE_DDR DDRC
    #define TRIGGER_ENABLE_PIN PC6
    #define TRIGGER_ENABLE_DD DDC6
    
    #define LED_PORT PORTD
    #define LED_DDR DDRD
    #define LED_GREEN_PIN PD4
    #define LED_GREEN_DD DDD4
    #define LED_RED_PIN PD7
    #define LED_RED_DD DDD7
    
    #define BLINKER_PORT PORTC
    #define BLINKER_DDR DDRC
    #define BLINKER_PIN PC7
    #define BLINKER_DD DDC7
#else
    #define TRIGGER_ENABLE_PORT PORTD
    #define TRIGGER_ENABLE_DDR DDRD
    #define TRIGGER_ENABLE_PIN PD4
    #define TRIGGER_ENABLE_DD DDD4

    #define LED_PORT PORTD
    #define LED_DDR DDRD
    #define LED_GREEN_PIN PD5
    #define LED_GREEN_DD DDD5
    #define LED_RED_PIN PD6
    #define LED_RED_DD DDD6

    #define BLINKER_PORT PORTB
    #define BLINKER_DDR DDRB
    #define BLINKER_PIN PB5
    #define BLINKER_DD DDB5
#endif

#define HEARTBEAT_DISABLED 0
#define HEARTBEAT_ENABLED 1
#define HEARTBEAT_TRIGGERED 2


// Number of seconds remaining until triggering the force-close
volatile uint8_t heartbeat = 0;

// Indicated whether the actively triggered and force-closing dome
volatile bool active = false;

// Sticky status for whether the heartbeat has triggered
// New heartbeat pings will be ignored if this is true 
volatile bool triggered = false;

// Number of close steps to send to the dome
volatile uint8_t shutter_a_close_steps = 0;
volatile uint8_t shutter_b_close_steps = 0;

// Rate limit the close bytes sent to the dome (2 per second)
volatile bool wait_before_next_byte = false;

static void set_status_led(uint8_t mode)
{
    switch(mode)
    {
        case HEARTBEAT_DISABLED:
            LED_PORT &= ~_BV(LED_GREEN_PIN);
            LED_PORT &= ~_BV(LED_RED_PIN);
        break;
        case HEARTBEAT_ENABLED:
            LED_PORT |= _BV(LED_GREEN_PIN);
            LED_PORT &= ~_BV(LED_RED_PIN);
        break;
        case HEARTBEAT_TRIGGERED:
            LED_PORT &= ~_BV(LED_GREEN_PIN);
            LED_PORT |= _BV(LED_RED_PIN);
        break;
    }
}

void tick()
{
    // Data is only recieved from the serial port if the heartbeat has triggered and we want to close
    // Therefore any status reporting an open shutter means we want to close that shutter.
    while (serial_can_read())
    {
        switch (serial_read())
        {
            case 'X': // 'A' shutter closed
                shutter_a_close_steps = 0;
                break;
            case 'Y': // 'B' shutter closed
                shutter_b_close_steps = 0;
                break;
            case '0': // 'A' shutter closed, 'B' shutter closed (PLC dome controller only)
                shutter_a_close_steps = 0;
                shutter_b_close_steps = 0;
        }
    }

    // Try and close the dome a step if we need to
    // If the heartbeat has triggered and the dome is not closed
    if (!wait_before_next_byte && (shutter_a_close_steps > 0 || shutter_b_close_steps > 0))
    {
        if (shutter_a_close_steps > 0)
        {
            serial_write('A');
            shutter_a_close_steps--;
        }
        else if (shutter_b_close_steps > 0)
        {
            serial_write('B');
            shutter_b_close_steps--;
        }

        wait_before_next_byte = true;
    }

    // Return serial control to the PC after both shutters are closed
    if (active && shutter_a_close_steps == 0 && shutter_b_close_steps == 0)
    {
        TRIGGER_ENABLE_PORT &= ~_BV(TRIGGER_ENABLE_PIN);
        active = false;
    }

    while (usb_can_read())
    {
        uint8_t value = usb_read();

        // Accept timeouts up to two minutes
        if (value > 240)
            continue;

        // Clear the sticky trigger flag when disabling the heartbeat
        // Also stops an active close
        if (value == 0)
        {
            triggered = false;
            active = false;
            TRIGGER_ENABLE_PORT &= ~_BV(TRIGGER_ENABLE_PIN);
        }

        // Update the heartbeat countdown (disabling it if 0)
        // If the heatbeat has triggered the status must be manually
        // cleared by sending a 0 byte
        if (!triggered)
        {
            heartbeat = value;
            set_status_led(heartbeat != 0 ? HEARTBEAT_ENABLED : HEARTBEAT_DISABLED);
        }
    }
}

int main()
{
    // Configure timer1 to interrupt every 0.50 seconds
    OCR1A = 7812;
    TCCR1B = _BV(CS12) | _BV(CS10) | _BV(WGM12);
    TIMSK1 |= _BV(OCIE1A);
    TRIGGER_ENABLE_DDR = _BV(TRIGGER_ENABLE_DD);

    // START V2 ONLY
    // Pin 13 is hard-wired to the onboard LED
    // Toggle it on the update timer to show the software is running
    // PD4 and PD6 are used for the status mode LEDs
    BLINKER_DDR |= _BV(BLINKER_DD);
    TRIGGER_ENABLE_DDR |= _BV(TRIGGER_ENABLE_DD);
    LED_DDR |= _BV(LED_GREEN_DD) | _BV(LED_RED_DD);

    set_status_led(HEARTBEAT_DISABLED);

    usb_initialize();
    serial_initialize();

    sei();
    for (;;)
        tick();
}

volatile bool led_active;
ISR(TIMER1_COMPA_vect)
{
    wait_before_next_byte = false;

    if ((led_active ^= true))
        BLINKER_PORT |= _BV(BLINKER_PIN);
    else
        BLINKER_PORT &= ~_BV(BLINKER_PIN);

    // 0xFF represents that the heartbeat has been tripped, and is sticky until the heartbeat is disabled
    // 0x00 represents that the heartbeat is disabled
    if (heartbeat != 0xFF && heartbeat != 0)
    {
        if (--heartbeat == 0)
        {
            shutter_a_close_steps = MAX_SHUTTER_CLOSE_STEPS;
            shutter_b_close_steps = MAX_SHUTTER_CLOSE_STEPS;

            set_status_led(HEARTBEAT_TRIGGERED);
            triggered = true;
            active = true;
            TRIGGER_ENABLE_PORT |= _BV(TRIGGER_ENABLE_PIN);
        }
    }

    // Update user on current status
    usb_write(active ? 254 : triggered ? 255 : heartbeat);
}
