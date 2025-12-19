#include "bus.h"
#include "cart.h"
#include "ppu.h"
#include "dma.h"
#include "apu.h"

enum adress_space
{
    RAM_BOUND           = 0x2000,
    PPUIO_BOUND         = 0x4000,
    OAM_DMA_IO          = 0x4014,
    JOY1_IO             = 0x4016,
    JOY2_IO             = 0x4017,
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
    OAM_DMA,
    JOY,
    APUIO,
    CARTIO,
};

static int bus_memory_map(u16 addr)
{
    if (addr < RAM_BOUND)           return RAM;
    else if (addr < PPUIO_BOUND)    return PPUIO;
    else if (addr == OAM_DMA_IO)    return OAM_DMA;
    else if (addr == JOY1_IO)       return JOY;
    else if (addr == JOY2_IO)       return JOY;
    else if (addr < APUIO_BOUND)    return APUIO;
    else if (addr < UNUSED_BOUND)   return UNKOWN;
    else                            return CARTIO;
}

static u8 __ram[0x800];
static u8 __controller[2] = {0, 0};
static u8 __controller_shift[2] = {0, 0};
static u8 __strobe = 0;

static void latch_controllers()
{
    __controller_shift[0] = __controller[0];
    __controller_shift[1] = __controller[1];
}


uint8_t bus_read(uint16_t addr) 
{
    switch (bus_memory_map(addr))
    {
    case RAM: return __ram[addr & 0x7FF];
    case PPUIO: return ppu_bus_read(addr);
    case JOY: {
        int id = addr & 1;

        // Read lowest bit
        u8 data = (__controller_shift[id] & 0x01);

        // Insert NES hardware bit6 = 1 (commonly returned)
        data |= 0x40;

        if (!__strobe)
        {
            // Shift only when strobe == 0
            __controller_shift[id] >>= 1;
            __controller_shift[id] |= 0x80; // after 8 reads, always 1
        }
        return data;
    }
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
    case OAM_DMA: dma_active(data, 0); break;
    case JOY: {
        if (addr == 0x4016)
        {
            u8 new_strobe = data & 1;

            // Detect 1 -> 0 transition
            if (__strobe == 1 && new_strobe == 0)
            {
                latch_controllers();
            }

            __strobe = new_strobe;

            // While strobe = 1, registers continuously reload
            if (__strobe)
            {
                latch_controllers();
            }
        }
        // NOTE: $4017 writes are NOT related to controller.
        return;
    }
    case APUIO: apu_bus_write(addr, data); break;
    case CARTIO: cart_write(addr, data); break;
    default: break;
    }
    return;
}

void bus_update_input(u8 p1, u8 p2)
{
    __controller[0] = p1;
    __controller[1] = p2;
}
