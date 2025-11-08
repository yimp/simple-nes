#include "bus.h"
#include "cart.h"
#include "ppu.h"

enum adress_space
{
    RAM_BOUND           = 0x2000,
    PPUIO_BOUND         = 0x4000,
    APUIO_BOUND         = 0x4018,
    UNUSED_BOUND        = 0x6000,
    WRAM_BOUND          = 0x8000,
    ROM_BOUND           = 0x0000,
};

enum cpu_io
{
    UNKOWN,
    RAM,
    PPUIO,
    APUIO,
    CARTIO,
};

static int bus_memory_map(u16 addr)
{
    if (addr < RAM_BOUND)           return RAM;
    else if (addr < PPUIO_BOUND)    return PPUIO;
    else if (addr < APUIO_BOUND)    return APUIO;
    else if (addr < UNUSED_BOUND)   return UNKOWN;
    else                            return CARTIO;
}

static u8 __ram[0x800];
uint8_t bus_read(uint16_t addr) 
{
    switch (bus_memory_map(addr))
    {
    case RAM: return __ram[addr & 0x7FF];
    case PPUIO: return ppu_bus_read(addr);
    case APUIO: return 0x00;
    case CARTIO: return cart_read(addr);
    default: break;
    }
    return 0x00;
}

void bus_write(uint16_t addr, uint8_t data) 
{
    switch (bus_memory_map(addr))
    {
    case RAM:  __ram[addr & 0x7FF] = data; return;
    case PPUIO: ppu_bus_write(addr, data); return;
    case APUIO: return;
    case CARTIO: cart_write(addr, data);
    default: break;
    }
    return;
}