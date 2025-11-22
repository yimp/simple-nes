#include "dma.h"
#include "ppu.h"
#include "bus.h"

typedef union 
{
struct
{
    u8 dma_page;
    u8 dma_data;
    u8 dma_addr;
    u8 dma_transfer     : 1;
    u8 cpu_halted       : 1;
    u8 clock_aligned    : 1;
    u8 write_clock      : 1;
    u8 unused           : 4;
};
    u32 clear;
} dma_ctx;

static dma_ctx __dma;

bool dma_processing()
{
    return __dma.dma_transfer;
}

void dma_clock()
{
    if (!__dma.cpu_halted) {
        __dma.cpu_halted = 1;
    } else if (__dma.clock_aligned) {
        __dma.clock_aligned = 1;
    } else {
        if (__dma.write_clock) {
            ppu_dma_oam_write(__dma.dma_addr++, __dma.dma_data);
            if (__dma.dma_addr == 0) {
                __dma.dma_transfer = 0;
                return;
            }
        } else {
            __dma.dma_data = bus_read((__dma.dma_page << 8) | __dma.dma_addr);
        }
        __dma.write_clock = ~__dma.write_clock;
    }
}

void dma_active(u8 dma_page, u8 clock_aligned)
{
    __dma.clear = 0;
    __dma.dma_transfer = 1;
    __dma.clock_aligned = clock_aligned & 0x01;
    __dma.dma_page = dma_page;
}