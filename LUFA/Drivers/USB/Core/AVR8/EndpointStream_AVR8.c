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
#include "../USBMode.h"

#include "EndpointStream_AVR8.h"

#if !defined(CONTROL_ONLY_DEVICE)

/* The following abuses the C preprocessor in order to copy-paste common code with slight alterations,
 * so that the code needs to be written once. It is a crude form of templating to reduce code maintenance. */

#define  TEMPLATE_FUNC_NAME                        Endpoint_Write_Stream_LE
#define  TEMPLATE_BUFFER_TYPE                      const void*
#define  TEMPLATE_CLEAR_ENDPOINT()                 Endpoint_ClearIN()
#define  TEMPLATE_BUFFER_OFFSET(Length)            0
#define  TEMPLATE_BUFFER_MOVE(BufferPtr, Amount)   BufferPtr += Amount
#define  TEMPLATE_TRANSFER_BYTE(BufferPtr)         Endpoint_Write_8(*BufferPtr)
#include "Template/Template_Endpoint_RW.c"

#endif

#define  TEMPLATE_FUNC_NAME                        Endpoint_Write_Control_Stream_LE
#define  TEMPLATE_BUFFER_OFFSET(Length)            0
#define  TEMPLATE_BUFFER_MOVE(BufferPtr, Amount)   BufferPtr += Amount
#define  TEMPLATE_TRANSFER_BYTE(BufferPtr)         Endpoint_Write_8(*BufferPtr)
#include "Template/Template_Endpoint_Control_W.c"

#define  TEMPLATE_FUNC_NAME                        Endpoint_Write_Control_PStream_LE
#define  TEMPLATE_BUFFER_OFFSET(Length)            0
#define  TEMPLATE_BUFFER_MOVE(BufferPtr, Amount)   BufferPtr += Amount
#define  TEMPLATE_TRANSFER_BYTE(BufferPtr)         Endpoint_Write_8(pgm_read_byte(BufferPtr))
#include "Template/Template_Endpoint_Control_W.c"

