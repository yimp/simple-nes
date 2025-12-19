#ifndef __APU_H__
#define __APU_H__
#include "types.h"

typedef struct {
    int        size;
    const i16* data;
} sample_vec;

// cpu bus io
u8   apu_bus_read(u16 addr);
void apu_bus_write(u16 addr, u8 data);

// basics
sample_vec apu_end_frame();
void apu_reset();

#endif