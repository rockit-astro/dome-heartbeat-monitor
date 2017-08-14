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
#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Common/Common.h>
#include "usb_v2_descriptors.h"

USB_ClassInfo_CDC_Device_t interface =
{
    .Config =
    {
        .ControlInterfaceNumber = INTERFACE_ID_CDC_CCI,
        .DataINEndpoint         =
        {
            .Address            = CDC_TX_EPADDR,
            .Size               = CDC_TXRX_EPSIZE,
            .Banks              = 1,
        },
        .DataOUTEndpoint        =
        {
            .Address            = CDC_RX_EPADDR,
            .Size               = CDC_TXRX_EPSIZE,
            .Banks              = 1,
        },
        .NotificationEndpoint   =
        {
            .Address            = CDC_NOTIFICATION_EPADDR,
            .Size               = CDC_NOTIFICATION_EPSIZE,
            .Banks              = 1,
        },
    },
};

#define USB_UNPLUGGED 0
#define USB_PLUGGED 1
#define USB_CONNECTED 2

static void set_status_led(uint8_t mode)
{
    switch(mode)
    {
        case USB_UNPLUGGED:
            PORTD &= ~_BV(PD0);
            PORTD &= ~_BV(PD1);
        break;
        case USB_PLUGGED:
            PORTD |= _BV(PD0);
            PORTD &= ~_BV(PD1);
        break;
        case USB_CONNECTED:
            PORTD &= ~_BV(PD0);
            PORTD |= _BV(PD1);
        break;
    }
}

void usb_initialize()
{
    // Status LEDs are attached to PD0 (usb connected), PD1 (USB plugged in)
    DDRD |= _BV(PD0) | _BV(PD1);

    // Wrap the stripped down LUFA CDC example code
    USB_Init();
}

bool usb_can_read()
{
    return CDC_Device_BytesReceived(&interface) > 0;
}

// Read a byte from the receive buffer
// Will block if the buffer is empty
uint8_t usb_read()
{
    return CDC_Device_ReceiveByte(&interface);
}

// Add a byte to the send buffer.
// Will block if the buffer is full
void usb_write(uint8_t b)
{
    // Note: This is ignoring any errors (e.g. send failed)
    // We are only sending single byte packets, so there's no
    // real benefits to handling them properly
    CDC_Device_SendByte(&interface, b);
    CDC_Device_Flush(&interface);
}

void EVENT_USB_Device_ConfigurationChanged(void)
{
    CDC_Device_ConfigureEndpoints(&interface);
}

void EVENT_CDC_Device_ControLineStateChanged(USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo)
{
    bool connected = CDCInterfaceInfo->State.ControlLineStates.HostToDevice & CDC_CONTROL_LINE_OUT_DTR;
    set_status_led(connected ? USB_CONNECTED : USB_PLUGGED);
}

void EVENT_USB_Device_Connect(void)
{
    set_status_led(USB_PLUGGED);
}

void EVENT_USB_Device_Disconnect(void)
{
    set_status_led(USB_UNPLUGGED);
}

void EVENT_USB_Device_ControlRequest(void)
{
    CDC_Device_ProcessControlRequest(&interface);
}
