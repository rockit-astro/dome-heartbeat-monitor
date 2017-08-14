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

/** \dir
 *  \brief Common library header files.
 *
 *  This folder contains header files which are common to all parts of the LUFA library. They may be used freely in
 *  user applications.
 */

/** \file
 *  \brief Common library convenience headers, macros and functions.
 *
 *  \copydetails Group_Common
 */

/** \defgroup Group_Common Common Utility Headers - LUFA/Drivers/Common/Common.h
 *  \brief Common library convenience headers, macros and functions.
 *
 *  Common utility headers containing macros, functions, enums and types which are common to all
 *  aspects of the library.
 *
 *  @{
 */

/** \defgroup Group_GlobalInt Global Interrupt Macros
 *  \brief Convenience macros for the management of interrupts globally within the device.
 *
 *  Macros and functions to create and control global interrupts within the device.
 */

#ifndef __LUFA_COMMON_H__
#define __LUFA_COMMON_H__

	/* Macros: */
		#define __INCLUDE_FROM_COMMON_H
		#define GCC_MEMORY_BARRIER() __asm__ __volatile__("" ::: "memory");

	/* Includes: */
		#include <stdint.h>
		#include <stdbool.h>
		#include <string.h>
		#include <stddef.h>

		#include "Attributes.h"

		#if defined(USE_LUFA_CONFIG_HEADER)
			#include "LUFAConfig.h"
		#endif

	/* Architecture specific utility includes: */

		#include <avr/io.h>
		#include <avr/interrupt.h>
		#include <avr/pgmspace.h>
		#include <avr/eeprom.h>
		#include <avr/boot.h>
		#include <math.h>
		#include <util/delay.h>

		typedef uint8_t uint_reg_t;

	/* Public Interface - May be used in end-application: */
			#if !defined(CONCAT) || defined(__DOXYGEN__)
				/** Concatenates the given input into a single token, via the C Preprocessor.
				 *
				 *  \param[in] x  First item to concatenate.
				 *  \param[in] y  Second item to concatenate.
				 *
				 *  \return Concatenated version of the input.
				 */
				#define CONCAT(x, y)            x ## y

				/** CConcatenates the given input into a single token after macro expansion, via the C Preprocessor.
				 *
				 *  \param[in] x  First item to concatenate.
				 *  \param[in] y  Second item to concatenate.
				 *
				 *  \return Concatenated version of the expanded input.
				 */
				#define CONCAT_EXPANDED(x, y)   CONCAT(x, y)
			#endif

			#if !defined(ISR) || defined(__DOXYGEN__)
				/** Macro for the definition of interrupt service routines, so that the compiler can insert the required
				 *  prologue and epilogue code to properly manage the interrupt routine without affecting the main thread's
				 *  state with unintentional side-effects.
				 *
				 *  Interrupt handlers written using this macro may still need to be registered with the microcontroller's
				 *  Interrupt Controller (if present) before they will properly handle incoming interrupt events.
				 *
				 *  \note This macro is only supplied on some architectures, where the standard library does not include a valid
				 *        definition. If an existing definition exists, the alternative definition here will be ignored.
				 *
				 *  \ingroup Group_GlobalInt
				 *
				 *  \param[in] Name  Unique name of the interrupt service routine.
				 */
				#define ISR(Name, ...)          void Name (void) __attribute__((__interrupt__)) __VA_ARGS__; void Name (void)
			#endif

		/* Inline Functions: */

			/** Retrieves a mask which contains the current state of the global interrupts for the device. This
			 *  value can be stored before altering the global interrupt enable state, before restoring the
			 *  flag(s) back to their previous values after a critical section using \ref SetGlobalInterruptMask().
			 *
			 *  \ingroup Group_GlobalInt
			 *
			 *  \return  Mask containing the current Global Interrupt Enable Mask bit(s).
			 */
			static inline uint_reg_t GetGlobalInterruptMask(void) ATTR_ALWAYS_INLINE ATTR_WARN_UNUSED_RESULT;
			static inline uint_reg_t GetGlobalInterruptMask(void)
			{
				GCC_MEMORY_BARRIER();
				return SREG;
			}

			/** Sets the global interrupt enable state of the microcontroller to the mask passed into the function.
			 *  This can be combined with \ref GetGlobalInterruptMask() to save and restore the Global Interrupt Enable
			 *  Mask bit(s) of the device after a critical section has completed.
			 *
			 *  \ingroup Group_GlobalInt
			 *
			 *  \param[in] GlobalIntState  Global Interrupt Enable Mask value to use
			 */
			static inline void SetGlobalInterruptMask(const uint_reg_t GlobalIntState) ATTR_ALWAYS_INLINE;
			static inline void SetGlobalInterruptMask(const uint_reg_t GlobalIntState)
			{
				GCC_MEMORY_BARRIER();
				SREG = GlobalIntState;
				GCC_MEMORY_BARRIER();
			}

			/** Enables global interrupt handling for the device, allowing interrupts to be handled.
			 *
			 *  \ingroup Group_GlobalInt
			 */
			static inline void GlobalInterruptEnable(void) ATTR_ALWAYS_INLINE;
			static inline void GlobalInterruptEnable(void)
			{
				GCC_MEMORY_BARRIER();
				sei();
				GCC_MEMORY_BARRIER();
			}

			/** Disabled global interrupt handling for the device, preventing interrupts from being handled.
			 *
			 *  \ingroup Group_GlobalInt
			 */
			static inline void GlobalInterruptDisable(void) ATTR_ALWAYS_INLINE;
			static inline void GlobalInterruptDisable(void)
			{
				GCC_MEMORY_BARRIER();
				cli();
				GCC_MEMORY_BARRIER();
			}

#endif

/** @} */

