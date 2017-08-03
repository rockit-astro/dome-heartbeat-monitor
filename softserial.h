//**********************************************************************************
//  Copyright 2017 Paul Chote
//  This file is part of dome-heartbeat-monitor, which is free software. It is made
//  available to you under version 3 (or later) of the GNU General Public License,
//  as published by the Free Software Foundation and included in the LICENSE file.
//**********************************************************************************

#include <stdarg.h>
#include <stdint.h>

#ifndef DOME_HEARTBEAT_SOFTSERIAL_H
#define DOME_HEARTBEAT_SOFTSERIAL_H

void softserial_initialize();
bool softserial_can_read();
uint8_t softserial_read();
void softserial_write(uint8_t b);

#endif
