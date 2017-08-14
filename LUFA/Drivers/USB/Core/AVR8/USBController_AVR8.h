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

/** \file
 *  \brief USB Controller definitions for the AVR8 microcontrollers.
 *  \copydetails Group_USBManagement_AVR8
 *
 *  \note This file should not be included directly. It is automatically included as needed by the USB driver
 *        dispatch header located in LUFA/Drivers/USB/USB.h.
 */

/** \ingroup Group_USBManagement
 *  \defgroup Group_USBManagement_AVR8 USB Interface Management (AVR8)
 *  \brief USB Controller definitions for the AVR8 microcontrollers.
 *
 *  Functions, macros, variables, enums and types related to the setup and management of the USB interface.
 *
 *  @{
 */

#ifndef __USBCONTROLLER_AVR8_H__
#define __USBCONTROLLER_AVR8_H__

	/* Includes: */
		#include "../../../../Common/Common.h"
		#include "../USBMode.h"
		#include "../Events.h"
		#include "../USBTask.h"
		#include "../USBInterrupt.h"

		#include "../Device.h"
		#include "../Endpoint.h"
		#include "../DeviceStandardReq.h"
		#include "../EndpointStream.h"

	/* Preprocessor Checks and Defines: */
		#if !defined(__INCLUDE_FROM_USB_DRIVER)
			#error Do not include this file directly. Include LUFA/Drivers/USB/USB.h instead.
		#endif

		#if !defined(F_USB)
			#error F_USB is not defined. You must define F_USB to the frequency of the unprescaled USB controller clock in your project makefile.
		#endif

		#if (F_USB == 16000000)
			#define USB_PLL_PSC                (1 << PINDIV)
		#endif

		#if !defined(USB_PLL_PSC)
			#error No PLL prescale value available for chosen F_USB value and AVR model.
		#endif

	/* Public Interface - May be used in end-application: */
		/* Macros: */
			/** \name USB Controller Option Masks */
			//@{
			/** Regulator disable option mask for \ref USB_Init(). This indicates that the internal 3.3V USB data pad
			 *  regulator should be disabled and the AVR's VCC level used for the data pads.
			 *
			 *  \note See USB AVR data sheet for more information on the internal pad regulator.
			 */
			#define USB_OPT_REG_DISABLED               (1 << 1)

			/** Regulator enable option mask for \ref USB_Init(). This indicates that the internal 3.3V USB data pad
			 *  regulator should be enabled to regulate the data pin voltages from the VBUS level down to a level within
			 *  the range allowable by the USB standard.
			 *
			 *  \note See USB AVR data sheet for more information on the internal pad regulator.
			 */
			#define USB_OPT_REG_ENABLED                (0 << 1)

			/** Option mask for \ref USB_Init() to keep regulator enabled at all times. Indicates that \ref USB_Disable()
			 *  should not disable the regulator as it would otherwise. Has no effect if regulator is disabled using
			 *  \ref USB_OPT_REG_DISABLED.
			 *
			 *  \note See USB AVR data sheet for more information on the internal pad regulator.
			 */
			#define USB_OPT_REG_KEEP_ENABLED           (1 << 3)

			/** Manual PLL control option mask for \ref USB_Init(). This indicates to the library that the user application
			 *  will take full responsibility for controlling the AVR's PLL (used to generate the high frequency clock
			 *  that the USB controller requires) and ensuring that it is locked at the correct frequency for USB operations.
			 */
			#define USB_OPT_MANUAL_PLL                 (1 << 2)

			/** Automatic PLL control option mask for \ref USB_Init(). This indicates to the library that the library should
			 *  take full responsibility for controlling the AVR's PLL (used to generate the high frequency clock
			 *  that the USB controller requires) and ensuring that it is locked at the correct frequency for USB operations.
			 */
			#define USB_OPT_AUTO_PLL                   (0 << 2)
			//@}

			#if !defined(USB_STREAM_TIMEOUT_MS) || defined(__DOXYGEN__)
				/** Constant for the maximum software timeout period of the USB data stream transfer functions
				 *  (both control and standard) when in either device or host mode. If the next packet of a stream
				 *  is not received or acknowledged within this time period, the stream function will fail.
				 *
				 *  This value may be overridden in the user project makefile as the value of the
				 *  \ref USB_STREAM_TIMEOUT_MS token, and passed to the compiler using the -D switch.
				 */
				#define USB_STREAM_TIMEOUT_MS       100
			#endif

		/* Inline Functions: */
			/** Determines if the VBUS line is currently high (i.e. the USB host is supplying power).
			 *
			 *  \note This function is not available on some AVR models which do not support hardware VBUS monitoring.
			 *
			 *  \return Boolean \c true if the VBUS line is currently detecting power from a host, \c false otherwise.
			 */
			static inline bool USB_VBUS_GetStatus(void) ATTR_WARN_UNUSED_RESULT ATTR_ALWAYS_INLINE;
			static inline bool USB_VBUS_GetStatus(void)
			{
				return ((USBSTA & (1 << VBUS)) ? true : false);
			}

			/** Detaches the device from the USB bus. This has the effect of removing the device from any
			 *  attached host, ceasing USB communications. If no host is present, this prevents any host from
			 *  enumerating the device once attached until \ref USB_Attach() is called.
			 */
			static inline void USB_Detach(void) ATTR_ALWAYS_INLINE;
			static inline void USB_Detach(void)
			{
				UDCON  |=  (1 << DETACH);
			}

			/** Attaches the device to the USB bus. This announces the device's presence to any attached
			 *  USB host, starting the enumeration process. If no host is present, attaching the device
			 *  will allow for enumeration once a host is connected to the device.
			 *
			 *  This is inexplicably also required for proper operation while in host mode, to enable the
			 *  attachment of a device to the host. This is despite the bit being located in the device-mode
			 *  register and despite the datasheet making no mention of its requirement in host mode.
			 */
			static inline void USB_Attach(void) ATTR_ALWAYS_INLINE;
			static inline void USB_Attach(void)
			{
				UDCON  &= ~(1 << DETACH);
			}

		/* Function Prototypes: */
			/** Main function to initialize and start the USB interface. Once active, the USB interface will
			 *  allow for device connection to a host when in device mode, or for device enumeration while in
			 *  host mode.
			 *
			 *  As the USB library relies on interrupts for the device and host mode enumeration processes,
			 *  the user must enable global interrupts before or shortly after this function is called. In
			 *  device mode, interrupts must be enabled within 500ms of this function being called to ensure
			 *  that the host does not time out whilst enumerating the device. In host mode, interrupts may be
			 *  enabled at the application's leisure however enumeration will not begin of an attached device
			 *  until after this has occurred.
			 *
			 *  Calling this function when the USB interface is already initialized will cause a complete USB
			 *  interface reset and re-enumeration.
			 *
			 *  \see \ref Group_Device for the \c USB_DEVICE_OPT_* masks.
			 */
			void USB_Init(void);

			/** Shuts down the USB interface. This turns off the USB interface after deallocating all USB FIFO
			 *  memory, endpoints and pipes. When turned off, no USB functionality can be used until the interface
			 *  is restarted with the \ref USB_Init() function.
			 */
			void USB_Disable(void);

			/** Resets the interface, when already initialized. This will re-enumerate the device if already connected
			 *  to a host, or re-enumerate an already attached device when in host mode.
			 */
			void USB_ResetInterface(void);

		/* Global Variables: */
			#define USB_CurrentMode USB_MODE_Device
			#define USB_Options USE_STATIC_OPTIONS

	/* Private Interface - For use in library only: */
	#if !defined(__DOXYGEN__)
		/* Function Prototypes: */
			#if defined(__INCLUDE_FROM_USB_CONTROLLER_C)
				static void USB_Init_Device(void);
			#endif

		/* Inline Functions: */
			static inline void USB_PLL_On(void) ATTR_ALWAYS_INLINE;
			static inline void USB_PLL_On(void)
			{
				PLLCSR = USB_PLL_PSC;
				PLLCSR = (USB_PLL_PSC | (1 << PLLE));
			}

			static inline void USB_PLL_Off(void) ATTR_ALWAYS_INLINE;
			static inline void USB_PLL_Off(void)
			{
				PLLCSR = 0;
			}

			static inline bool USB_PLL_IsReady(void) ATTR_WARN_UNUSED_RESULT ATTR_ALWAYS_INLINE;
			static inline bool USB_PLL_IsReady(void)
			{
				return ((PLLCSR & (1 << PLOCK)) ? true : false);
			}

			static inline void USB_REG_On(void) ATTR_ALWAYS_INLINE;
			static inline void USB_REG_On(void)
			{
				UHWCON |=  (1 << UVREGE);
			}

			static inline void USB_REG_Off(void) ATTR_ALWAYS_INLINE;
			static inline void USB_REG_Off(void)
			{
				UHWCON &= ~(1 << UVREGE);
			}

			static inline void USB_OTGPAD_On(void) ATTR_ALWAYS_INLINE;
			static inline void USB_OTGPAD_On(void)
			{
				USBCON |=  (1 << OTGPADE);
			}

			static inline void USB_OTGPAD_Off(void) ATTR_ALWAYS_INLINE;
			static inline void USB_OTGPAD_Off(void)
			{
				USBCON &= ~(1 << OTGPADE);
			}

			static inline void USB_CLK_Freeze(void) ATTR_ALWAYS_INLINE;
			static inline void USB_CLK_Freeze(void)
			{
				USBCON |=  (1 << FRZCLK);
			}

			static inline void USB_CLK_Unfreeze(void) ATTR_ALWAYS_INLINE;
			static inline void USB_CLK_Unfreeze(void)
			{
				USBCON &= ~(1 << FRZCLK);
			}

			static inline void USB_Controller_Enable(void) ATTR_ALWAYS_INLINE;
			static inline void USB_Controller_Enable(void)
			{
				USBCON |=  (1 << USBE);
			}

			static inline void USB_Controller_Disable(void) ATTR_ALWAYS_INLINE;
			static inline void USB_Controller_Disable(void)
			{
				USBCON &= ~(1 << USBE);
			}

			static inline void USB_Controller_Reset(void) ATTR_ALWAYS_INLINE;
			static inline void USB_Controller_Reset(void)
			{
				USBCON &= ~(1 << USBE);
				USBCON |=  (1 << USBE);
			}
	#endif

#endif

/** @} */

