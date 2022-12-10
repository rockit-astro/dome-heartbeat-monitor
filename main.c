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

#define RELAY_DISABLED PORTC &= ~_BV(PC6)
#define RELAY_ENABLED  PORTC |= _BV(PC6)
#define RELAY_INIT     DDRC |= _BV(DDC6), RELAY_DISABLED

#if EXTERNAL_SIREN
#define SIREN_DISABLED PORTB &= ~_BV(PB2)
#define SIREN_ENABLED  PORTB |= _BV(PB2)
#define SIREN_INIT     DDRB |= _BV(DDB2), SIREN_DISABLED
#else
#define SIREN_DISABLED PORTE &= ~_BV(PE6)
#define SIREN_ENABLED  PORTE |= _BV(PE6)
#define SIREN_INIT     DDRE |= _BV(DDE6), SIREN_DISABLED
#endif

#define BLINKER_LED_DISABLED PORTC &= ~_BV(PC7)
#define BLINKER_LED_ENABLED  PORTC |= _BV(PC7)
#define BLINKER_LED_INIT     DDRC |= _BV(DDC7), BLINKER_LED_DISABLED

#define HEARTBEAT_LED_DISABLED  PORTD &= ~_BV(PD4), PORTD &= ~_BV(PD7)
#define HEARTBEAT_LED_ENABLED   PORTD |= _BV(PD4), PORTD &= ~_BV(PD7)
#define HEARTBEAT_LED_TRIGGERED PORTD &= ~_BV(PD4), PORTD |= _BV(PD7)
#define HEARTBEAT_LED_INIT      DDRD |= _BV(DDD4) | _BV(DDD7), HEARTBEAT_LED_DISABLED

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
volatile uint8_t relay_reset_steps = 0;
volatile uint8_t enable_siren_steps = 0;

// Rate limit the status reports to the host PC to 2Hz
volatile bool send_status_byte = false;

void poll_usb(void)
{
    // Check for ping or disable bytes from the host PC
    while (usb_can_read())
    {
        int16_t value = usb_read();
        if (value < 0)
            break;

        // Enable the siren for 5 seconds
        if (value == 0xFF)
            enable_siren_steps = 10;

        // Accept timeouts up to two minutes
        if (value > 240)
            continue;

        // Clear the sticky trigger flag when disabling the heartbeat
        // Also stops an active close
        if (value == 0)
        {
            triggered = false;
            active = false;
            RELAY_DISABLED;
        }

        // Update the heartbeat countdown (disabling it if 0)
        // If the heatbeat has triggered the status must be manually
        // cleared by sending a 0 byte
        if (!triggered)
        {
            heartbeat = value;
            if (heartbeat != 0)
                HEARTBEAT_LED_ENABLED;
            else
                HEARTBEAT_LED_DISABLED;
        }
    }

    if (send_status_byte)
    {
        // Send current status back to the host computer
        usb_write(active ? 254 : triggered ? 255 : heartbeat);
        send_status_byte = false;
    }
}

int main(void)
{
    // Configure timer1 to interrupt every 0.50 seconds
    OCR1A = 7812;
    TCCR1B = _BV(CS12) | _BV(CS10) | _BV(WGM12);
    TIMSK1 |= _BV(OCIE1A);

    RELAY_INIT;
    SIREN_INIT;
    BLINKER_LED_INIT;
    HEARTBEAT_LED_INIT;

    usb_initialize();
    serial_initialize();

    sei();
    for (;;)
        poll_usb();
}

volatile bool led_active;
ISR(TIMER1_COMPA_vect)
{
    // Update the status LED on the arduino to show that the
    // monitoring is still active
    if ((led_active ^= true))
        BLINKER_LED_ENABLED;
    else
        BLINKER_LED_DISABLED;

    // Check whether we need to close the dome
    // This is done inside the ISR to avoid any problems with the USB connection blocking
    // from interfering with the primary job of the device

    // Check for data from the dome PLC
    // This port is physically disconnected until the relay is enabled
    // so we only expect data to be read when we are tripped and actively closing
    //
    // IMPORTANT: We must interact with the PLC serial from within this ISR because
    // the USB read/write can block the main thread if the connection goes down uncleanly
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

    // Decrement the heartbeat counter and trigger a close if it reaches 0
    // There are two special values that are not real times:
    //   0xFF represents that the heartbeat has been tripped, and is sticky until the heartbeat is disabled
    //   0x00 represents that the heartbeat is disabled
    if (heartbeat != 0xFF && heartbeat != 0)
    {
        // Start the siren 5 seconds before closing
        // There is a chance that we may receive another
        // ping, but its much more likely that we will close
        if (heartbeat == 10)
            enable_siren_steps = 10;

        if (--heartbeat == 0)
        {
            shutter_a_close_steps = MAX_SHUTTER_CLOSE_STEPS;
            shutter_b_close_steps = MAX_SHUTTER_CLOSE_STEPS;

            HEARTBEAT_LED_TRIGGERED;
            triggered = true;
            active = true;
            RELAY_ENABLED;

            #if HAS_BUMPER_GUARD
            // Spend a couple of seconds trying to toggle
            // the bumper guard relay before sending close commands
            relay_reset_steps = 4;
            #endif
        }
    }

    if (relay_reset_steps > 0)
    {
        serial_write('R');
        relay_reset_steps--;
    }
    else if (shutter_a_close_steps > 0)
    {
        // Close the dome by a step
        serial_write('A');
        shutter_a_close_steps--;
    }
    else if (shutter_b_close_steps > 0)
    {
        serial_write('B');
        shutter_b_close_steps--;
    }

    // Return serial control to the PC after both shutters are closed
    if (active && shutter_a_close_steps == 0 && shutter_b_close_steps == 0)
    {
        RELAY_DISABLED;
        active = false;
    }

    if (enable_siren_steps > 0)
    {
        SIREN_ENABLED;

        if (--enable_siren_steps == 0)
            SIREN_DISABLED;
    }

    send_status_byte = true;
}
