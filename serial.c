//**********************************************************************************
//  Copyright 2016, 2017 Paul Chote
//  This file is part of dome-heartbeat-monitor, which is free software. It is made
//  available to you under version 3 (or later) of the GNU General Public License,
//  as published by the Free Software Foundation and included in the LICENSE file.
//**********************************************************************************

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <stdint.h>

static uint8_t output_buffer[256];
static volatile uint8_t output_read = 0;
static volatile uint8_t output_write = 0;

static uint8_t input_buffer[256];
static uint8_t input_read = 0;
static volatile uint8_t input_write = 0;

void serial_initialize(void)
{
#define BAUD 9600
#include <util/setbaud.h>
    UBRR1H = UBRRH_VALUE;
    UBRR1L = UBRRL_VALUE;
#if USE_2X
    UCSR1A = _BV(U2X0);
#endif

    // Enable receive, transmit, data received interrupt
    UCSR1B = _BV(RXEN1) | _BV(TXEN1) | _BV(RXCIE1);

    input_read = input_write = 0;
    output_read = output_write = 0;
}

bool serial_can_read(void)
{
    return input_write != input_read;
}

// Read a byte from the receive buffer
// Will block if the buffer is empty
uint8_t serial_read(void)
{
    while (input_read == input_write);
    return input_buffer[input_read++];
}

// Add a byte to the send buffer.
// Will block if the buffer is full
void serial_write(uint8_t b)
{
    // Don't overwrite data that hasn't been sent yet
    while (output_write == (uint8_t)(output_read - 1));

    output_buffer[output_write++] = b;

    // Enable transmit if necessary
    UCSR1B |= _BV(UDRIE1);
}

ISR(USART1_UDRE_vect)
{
    if (output_write != output_read)
        UDR1 = output_buffer[output_read++];

    // Ran out of data to send - disable the interrupt
    if (output_write == output_read)
        UCSR1B &= ~_BV(UDRIE1);
}

ISR(USART1_RX_vect)
{
    input_buffer[(uint8_t)(input_write++)] = UDR1;
}
