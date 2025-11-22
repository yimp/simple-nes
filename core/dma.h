#ifndef __DMA_H__
#define __DMA_H__
#include "types.h"

bool dma_processing();
void dma_clock();
void dma_active(u8 dma_page, u8 clock_aligned);

#endif