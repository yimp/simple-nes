#ifndef __BUS_H__
#define __BUS_H__
#include "types.h"

// cpu I/O
uint8_t bus_read(uint16_t addr);
void    bus_write(uint16_t addr, uint8_t data);

// basics
void bus_update_input(u8 p1, u8 p2);
#endif