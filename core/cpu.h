#ifndef __CPU_H__
#define __CPU_H__
#include "types.h"

bool cpu_clock();
void cpu_reset();
void cpu_nmi();


// test
u16 cpu_x_y();

#endif