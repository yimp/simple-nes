#include "../mapper.h"
#include "../cart.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    u8 reg[8];
} mapper246_t;


u8 mapper246_cpu_read(nes_rom_info* context, u16 addr)
{
    mapper246_t* self = GET_SELF(mapper246_t, context->mapper);
    if (addr >= 0x6800 && addr <= 0x6FFF)
	{
		return context->battery_ram[addr & 0x07FF];
	}

    int prg_bank = self->reg[(addr >> 13) & 0x3] & 0x3F;
    if (addr > 0xFFE4 && ((1 << (addr - 0xFFE4)) & 0x0F0F0F0F))
    {
        prg_bank |= 0x10;
        self->reg[(addr >> 13) & 0x3] |= 0x10;
    }
    return context->prg_area[(prg_bank << 13) | (addr & 0x1FFF)];
}

void mapper246_cpu_write(nes_rom_info* context, u16 addr, u8 data)
{
    mapper246_t* self = GET_SELF(mapper246_t, context->mapper);
    if (addr >= 0x6000 && addr < 0x67FF) {
        self->reg[addr & 0x7] = data;
    } 
    else if(addr >= 0x6800 && addr <= 0x6FFF)
	{
		context->battery_ram[addr & 0x07FF] = data;
	}
}

u8 mapper246_ppu_read(nes_rom_info* context, u16 addr)
{
    mapper246_t* self = GET_SELF(mapper246_t, context->mapper);
    int chr_bank = self->reg[0x4 | ((addr >> 11) & 0x3)];
    return context->chr_area[(chr_bank << 11) | (addr & 0x07FF)];

    // return context->prg_area[(self->reg[0x4 | (((addr >> 11)) & 0x3)] << 11) | (addr & 0x07FF)];
}


void mapper246_init(nes_rom_info* context, mapper_t* mapper)
{
    mapper->priv = malloc(sizeof(mapper246_t));
    memset(mapper->priv, 0x00, sizeof(mapper246_t));
    mapper246_t* self = GET_SELF(mapper246_t, mapper);
    self->reg[0] = 0;
    self->reg[1] = 1;
    self->reg[2] = 2;
    self->reg[3] = 0xFF;
    self->reg[4 + 0] = 0;
    self->reg[4 + 1] = 1;
    self->reg[4 + 2] = 2;
    self->reg[4 + 3] = 3;
    mapper->cpu_read = mapper246_cpu_read;
    mapper->cpu_write = mapper246_cpu_write;
    mapper->ppu_read = mapper246_ppu_read;
}