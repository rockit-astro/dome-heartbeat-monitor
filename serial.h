//**********************************************************************************
//  Copyright 2016, 2017 Paul Chote
//  This file is part of dome-heartbeat-monitor, which is free software. It is made
//  available to you under version 3 (or later) of the GNU General Public License,
//  as published by the Free Software Foundation and included in the LICENSE file.
//**********************************************************************************

#include <stdarg.h>
#include <stdint.h>

#ifndef DOME_HEARTBEAT_SERIAL_H
#define DOME_HEARTBEAT_SERIAL_H

void serial_initialize(void);
bool serial_can_read(void);
uint8_t serial_read(void);
void serial_write(uint8_t b);

#endif
