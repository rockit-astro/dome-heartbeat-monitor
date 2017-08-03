//**********************************************************************************
//  Copyright 2017 Paul Chote
//  This file is part of dome-heartbeat-monitor, which is free software. It is made
//  available to you under version 3 (or later) of the GNU General Public License,
//  as published by the Free Software Foundation and included in the LICENSE file.
//**********************************************************************************

// Technique and timings for bit-bashing serial data is adapted from SoftwareSerial.cpp,
// which is available under the terms of the GNU LGPL V2.1

#include <avr/interrupt.h>
#include <stdbool.h>

// Timings for delay_tuned to bit-bash 9600 baud serial
// Replace these with the appropriate values from SoftwareSerial.cpp
// if you want to change the effective baud rate
const int RX_DELAY_CENTERING = 114;
const int RX_DELAY_INTRABIT = 236;
const int RX_DELAY_STOPBIT = 236;
const int TX_DELAY = 233;
const int TX_START_OFFSET = 5;

// TX on pin 10 (XXX), RX on pin 11 (XXX)
#define SOFTSERIAL_PORT PORTD
#define SOFTSERIAL_PINREG PIND
#define SOFTSERIAL_DDR DDRD

// Note that this relies on the INT0 interrupt on PD2
#define SOFTSERIAL_RX_PIN PD2
#define SOFTSERIAL_TX_PIN PD3
#define SOFTSERIAL_TX_DD DDD3

static uint8_t input_buffer[256];
static uint8_t input_read = 0;
static volatile uint8_t input_write = 0;

static void delay_tuned(uint16_t delay)
{
    uint8_t tmp = 0;
    asm volatile("sbiw    %0, 0x01 \n\t"
        "ldi %1, 0xFF \n\t"
        "cpi %A0, 0xFF \n\t"
        "cpc %B0, %1 \n\t"
        "brne .-10 \n\t"
        : "+r" (delay), "+a" (tmp)
        : "0" (delay)
        );
}

ISR(INT0_vect)
{
    uint8_t d = 0;

    // Make sure that the line is still low, indicating that this is a new trigger
    // and not a stale trigger the last recieved byte
    if (bit_is_clear(SOFTSERIAL_PINREG, SOFTSERIAL_RX_PIN))
    {
        // Wait approximately 1/2 of a bit width to "center" the sample
        delay_tuned(RX_DELAY_CENTERING);

        // Read each of the 8 bits
        for (uint8_t i = 0x1; i; i <<= 1)
        {
            delay_tuned(RX_DELAY_INTRABIT);
            uint8_t noti = ~i;
            if (bit_is_set(SOFTSERIAL_PINREG, SOFTSERIAL_RX_PIN))
                d |= i;
            else // else clause added to ensure function timing is ~balanced
                d &= noti;
        }

        // skip the stop bit
        // TODO: verify stop bit
        delay_tuned(RX_DELAY_STOPBIT);

        // TODO: Checksum

        input_buffer[(uint8_t)(input_write++)] = d;
    }
}

void softserial_initialize()
{
    // Configure output pins
    // Don't overwrite other bits in the register!
    SOFTSERIAL_DDR |= _BV(SOFTSERIAL_TX_DD);
    SOFTSERIAL_PORT &= ~_BV(SOFTSERIAL_TX_PIN);

    // Enable pullup resistor on RX input
    SOFTSERIAL_PORT |= _BV(SOFTSERIAL_RX_PIN);

    // Set INT0 to be falling edge triggered
    EICRA = _BV(ISC01);
    EIMSK |= _BV(INT0);

    delay_tuned(TX_DELAY);
}

bool softserial_can_read()
{
    return input_write != input_read;
}

// Read a byte from the receive buffer
// Blocks if the buffer is empty
uint8_t softserial_read()
{
    while (input_read == input_write);
    return input_buffer[input_read++];
}

static void set_tx_pin(uint8_t high)
{
    if (high)
      SOFTSERIAL_PORT |= _BV(SOFTSERIAL_TX_PIN);
    else
      SOFTSERIAL_PORT &= ~_BV(SOFTSERIAL_TX_PIN);
}

// Send a byte
// Blocks until sending is complete
void softserial_write(uint8_t b)
{
    uint8_t sreg = SREG;

    // Disable interrupts while sending
    cli();

    // Start bit
    set_tx_pin(0);
    delay_tuned(TX_DELAY + TX_START_OFFSET);

    // Data bits
    for (uint8_t mask = 0x01; mask; mask <<= 1)
    {
        set_tx_pin(b & mask);
        delay_tuned(TX_DELAY);
    }

    // TODO: Checksum

    set_tx_pin(1); // restore pin to natural state

    // Restore interrupts
    SREG = sreg;
    delay_tuned(TX_DELAY);
}
