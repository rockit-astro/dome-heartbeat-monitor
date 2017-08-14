/*
             LUFA Library
     Copyright (C) Dean Camera, 2017.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2017  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

#include "../../../../Common/Common.h"

#define  __INCLUDE_FROM_USB_DRIVER
#define  __INCLUDE_FROM_USB_CONTROLLER_C
#include "../USBController.h"

void USB_Init(
               void
               )
{
	/* Workaround for AVR8 bootloaders that fail to turn off the OTG pad before running
	 * the loaded application. This causes VBUS detection to fail unless we first force
	 * it off to reset it. */
	USB_OTGPAD_Off();

	if (!(USB_Options & USB_OPT_REG_DISABLED))
	  USB_REG_On();
	else
	  USB_REG_Off();

	if (!(USB_Options & USB_OPT_MANUAL_PLL))
	{
		PLLFRQ = (1 << PDIV2);
	}

	USB_IsInitialized = true;

	USB_ResetInterface();
}

void USB_Disable(void)
{
	USB_INT_DisableAllInterrupts();
	USB_INT_ClearAllInterrupts();

	USB_Detach();
	USB_Controller_Disable();

	if (!(USB_Options & USB_OPT_MANUAL_PLL))
	  USB_PLL_Off();

	if (!(USB_Options & USB_OPT_REG_KEEP_ENABLED))
	  USB_REG_Off();

	USB_OTGPAD_Off();

	USB_IsInitialized = false;
}

void USB_ResetInterface(void)
{
	USB_INT_DisableAllInterrupts();
	USB_INT_ClearAllInterrupts();

	USB_Controller_Reset();

	USB_CLK_Unfreeze();

	if (USB_CurrentMode == USB_MODE_Device)
	{
		if (!(USB_Options & USB_OPT_MANUAL_PLL))
		{
			USB_PLL_Off();
		}

		USB_Init_Device();
	}

	USB_OTGPAD_On();
}

static void USB_Init_Device(void)
{
	USB_DeviceState                 = DEVICE_STATE_Unattached;
	USB_Device_ConfigurationNumber  = 0;

	#if !defined(NO_DEVICE_REMOTE_WAKEUP)
	USB_Device_RemoteWakeupEnabled  = false;
	#endif

	#if !defined(NO_DEVICE_SELF_POWER)
	USB_Device_CurrentlySelfPowered = false;
	#endif

	#if !defined(FIXED_CONTROL_ENDPOINT_SIZE)
	USB_Descriptor_Device_t* DeviceDescriptorPtr;

	if (CALLBACK_USB_GetDescriptor((DTYPE_Device << 8), 0, (void*)&DeviceDescriptorPtr) != NO_DESCRIPTOR)
	{
		USB_Device_ControlEndpointSize = pgm_read_byte(&DeviceDescriptorPtr->Endpoint0Size);
	}
	#endif

	if (USB_Options & USB_DEVICE_OPT_LOWSPEED)
	  USB_Device_SetLowSpeed();
	else
	  USB_Device_SetFullSpeed();

	USB_INT_Enable(USB_INT_VBUSTI);

	Endpoint_ConfigureEndpoint(ENDPOINT_CONTROLEP, EP_TYPE_CONTROL,
							   USB_Device_ControlEndpointSize, 1);

	USB_INT_Clear(USB_INT_SUSPI);
	USB_INT_Enable(USB_INT_SUSPI);
	USB_INT_Enable(USB_INT_EORSTI);

	USB_Attach();
}
