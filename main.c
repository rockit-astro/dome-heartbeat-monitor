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
#include "softserial.h"

#define TRIGGER_ENABLE_PORT PORTD
#define TRIGGER_ENABLE_PINREG PIND
#define TRIGGER_ENABLE_DDR DDRD
#define TRIGGER_ENABLE_PIN PD4
#define TRIGGER_ENABLE_DD DDD4

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

void tick()
{
    // Data is only recieved from the serial port if the heartbeat has triggered and we want to close
    // Therefore any status reporting an open shutter means we want to close that shutter.
    while (softserial_can_read())
    {
        switch (softserial_read())
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
            softserial_write('A');
            shutter_a_close_steps--;
        }
        else if (shutter_b_close_steps > 0)
        {
            softserial_write('B');
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
            heartbeat = value;
    }
}

int main()
{
    // Configure timer1 to interrupt every 0.50 seconds
    OCR1A = 7812;
    TCCR1B = _BV(CS12) | _BV(CS10) | _BV(WGM12);
    TIMSK1 |= _BV(OCIE1A);
    TRIGGER_ENABLE_DDR = _BV(TRIGGER_ENABLE_DD);

    usb_initialize();
    softserial_initialize();

    sei();
    for (;;)
        tick();
}

ISR(TIMER1_COMPA_vect)
{
    wait_before_next_byte = false;

    // 0xFF represents that the heartbeat has been tripped, and is sticky until the heartbeat is disabled
    // 0x00 represents that the heartbeat is disabled
    if (heartbeat != 0xFF && heartbeat != 0)
    {
        if (--heartbeat == 0)
        {
            shutter_a_close_steps = MAX_SHUTTER_CLOSE_STEPS;
            shutter_b_close_steps = MAX_SHUTTER_CLOSE_STEPS;

            triggered = true;
            active = true;
            TRIGGER_ENABLE_PORT |= _BV(TRIGGER_ENABLE_PIN);
        }
    }

    // Update user on current status
    usb_write(active ? 254 : triggered ? 255 : heartbeat);
}
