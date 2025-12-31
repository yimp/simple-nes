#include "../mapper.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    u8  reg; // bank select
    u8* chr;
} uxrom_t;

u8  uxrom_cpu_read(nes_rom_info* context, u16 addr)
{
    uxrom_t* self = GET_SELF(uxrom_t, context->mapper);
	if (addr >= 0x8000 && addr <= 0xBFFF)
	{
		return context->prg_area[(self->reg << 14) | (addr & 0x3FFF)];
	}

	if (addr >= 0xC000 && addr <= 0xFFFF)
	{
		return context->prg_area[((context->prg_rom_pages - 1) << 14) | (addr & 0x3FFF)];
	}
    return 0x00;
}

void uxrom_cpu_write(nes_rom_info* context, u16 addr, u8 data)
{
    uxrom_t* self = GET_SELF(uxrom_t, context->mapper);
    if (addr < MAPPER_BASE)
        return; // no wram
    self->reg = data;
}


u8   uxrom_ppu_read(nes_rom_info* context, u16 addr) 
{
    uxrom_t* self = GET_SELF(uxrom_t, context->mapper);
    if (self->chr)
        return self->chr[addr & 0x1FFF];
    return context->chr_area[addr & 0x1FFF];
}

void uxrom_ppu_write(nes_rom_info* context, u16 addr, u8 data) 
{
    uxrom_t* self = GET_SELF(uxrom_t, context->mapper);
    if (self->chr)
        self->chr[addr & 0x1FFF] = data;
}

void uxrom_init(nes_rom_info* context, mapper_t* mapper)
{
    mapper->cpu_read = uxrom_cpu_read;
    mapper->cpu_write = uxrom_cpu_write;
    mapper->ppu_read = uxrom_ppu_read;
    mapper->ppu_write = uxrom_ppu_write;
    mapper->priv = malloc(sizeof(uxrom_t));
    memset(mapper->priv, 0x00, sizeof(uxrom_t));
    uxrom_t* self = GET_SELF(uxrom_t, mapper);

    if (context->chr_rom_pages == 0)
    {
        self->chr = malloc(8192);
    } else {
        self->chr = NULL;
    }
}


void uxrom_destroy(nes_rom_info* context, mapper_t* mapper)
{
    uxrom_t* self = GET_SELF(uxrom_t, mapper);
    if (self && context->chr_rom_pages == 0) {
        free(self->chr);
        free(mapper->priv);
        mapper->priv = NULL;
    }
}
