#ifndef __PPU_H__
#define __PPU_H__
#include "types.h"

// cpu bus io
u8   ppu_bus_read(u16 addr);
void ppu_bus_write(u16 addr, u8 data);

// basics
void ppu_clock();
bool ppu_nmi_triggered();
u16* ppu_frame_buffer();

#endif