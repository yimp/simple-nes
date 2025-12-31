#include "../mapper.h"
#include "../cart.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    u8 mirroring_mode;
    u8 prg_ram[0x2000];
    const u8* prg_banks[4];
    const u8* chr_banks[8];

    u8 irq_counter;
    u8 irq_reload;
    bool irq_active;
    bool irq_enable;
    bool irq_update;

    u8 chr_inversion;
    u8 target_register;
    u8 prg_bank_mode;
    u32 registers[8];
} mmc3_t;

u8 mmc3_cpu_read(nes_rom_info* context, u16 addr)
{
    mmc3_t* priv = GET_SELF(mmc3_t, context->mapper);

    if (addr >= 0x6000 && addr <= 0x7FFF)
	{
		return context->battery_ram[addr & 0x1FFF];
	}
    return priv->prg_banks[(addr & 0x6000) >> 13][addr & 0x1FFF];
}

void mmc3_cpu_write(nes_rom_info* context, u16 addr, u8 data)
{
    mmc3_t* priv = GET_SELF(mmc3_t, context->mapper);
    if (addr >= 0x6000 && addr <= 0x7FFF)
	{
		if (addr == 0x67F0 || addr == 0x67F1)
		{
			int a = 0;
		}
		context->battery_ram[addr & 0x1FFF] = data;
	}

	if (addr >= 0x8000 && addr <= 0x9FFF)
	{
		// Bank Select
		if (!(addr & 0x0001))
		{
			priv->target_register = data & 0x07;
			priv->prg_bank_mode   = (data & 0x40);
			priv->chr_inversion   = (data & 0x80);
		}
		else
		{
			// Update target register
			priv->registers[priv->target_register] = data;

			// Update Pointer Table
			if (priv->chr_inversion)
			{
				priv->chr_banks[0] = context->chr_area + (priv->registers[2] << 10);
				priv->chr_banks[1] = context->chr_area + (priv->registers[3] << 10);
				priv->chr_banks[2] = context->chr_area + (priv->registers[4] << 10);
				priv->chr_banks[3] = context->chr_area + (priv->registers[5] << 10);
				priv->chr_banks[4] = context->chr_area + ((priv->registers[0] & 0xFE) << 10);
				priv->chr_banks[5] = context->chr_area + ((priv->registers[0] + 1) << 10);
				priv->chr_banks[6] = context->chr_area + ((priv->registers[1] & 0xFE) << 10);
				priv->chr_banks[7] = context->chr_area + ((priv->registers[1] + 1) << 10);
			}
			else
			{
				priv->chr_banks[0] = context->chr_area + ((priv->registers[0] & 0xFE) << 10);
				priv->chr_banks[1] = context->chr_area + ((priv->registers[0] + 1) << 10);
				priv->chr_banks[2] = context->chr_area + ((priv->registers[1] & 0xFE) << 10);
				priv->chr_banks[3] = context->chr_area + ((priv->registers[1] + 1) << 10);
				priv->chr_banks[4] = context->chr_area + (priv->registers[2] << 10);
				priv->chr_banks[5] = context->chr_area + (priv->registers[3] << 10);
				priv->chr_banks[6] = context->chr_area + (priv->registers[4] << 10);
				priv->chr_banks[7] = context->chr_area + (priv->registers[5] << 10);
			}

			if (priv->prg_bank_mode)
			{
				priv->prg_banks[2] = context->prg_area + ((priv->registers[6] & 0x3F) << 13);
				priv->prg_banks[0] = context->prg_area + (((context->prg_rom_pages << 1) - 2) << 13);
			}
			else
			{
                priv->prg_banks[0] = context->prg_area + ((priv->registers[6] & 0x3F) << 13);
				priv->prg_banks[2] = context->prg_area + (((context->prg_rom_pages << 1) - 2) << 13);
			}

			priv->prg_banks[1] = context->prg_area + ((priv->registers[7] & 0x3F) << 13);
            priv->prg_banks[3] = context->prg_area + (((context->prg_rom_pages << 1) - 1) << 13);
		}
	}

	if (addr >= 0xA000 && addr <= 0xBFFF)
	{
		if (!(addr & 0x0001))
		{
			// Mirroring
			if (data & 0x01)
				priv->mirroring_mode = CartMirrorHorizontal;
			else
				priv->mirroring_mode = CartMirrorVertical;
		}
		else
		{
            // TODO: PRG Ram Protect
		}
	}

	if (addr >= 0xC000 && addr <= 0xDFFF)
	{
		if (!(addr & 0x0001))
		{
			priv->irq_reload = data;
		}
		else
		{
			priv->irq_counter = 0x0000;
		}
	}

	if (addr >= 0xE000 && addr <= 0xFFFF)
	{
		if (!(addr & 0x0001))
		{
			priv->irq_enable = false;
			priv->irq_active = false;
		}
		else
		{
			priv->irq_enable = true;
		}
	}

}

u8 mmc3_ppu_read(nes_rom_info* context, u16 addr)
{
    mmc3_t* priv = GET_SELF(mmc3_t, context->mapper);
    return priv->chr_banks[(addr & 0x1C00) >> 10][addr & 0x03FF];
}

u8 mmc3_get_mirror(nes_rom_info* context)
{
    mmc3_t* priv = GET_SELF(mmc3_t, context->mapper);
    return priv->mirroring_mode;
}

void mmc3_scanline_handler(nes_rom_info* context)
{
    mmc3_t* priv = GET_SELF(mmc3_t, context->mapper);
	if (priv->irq_counter == 0)
		priv->irq_counter = priv->irq_reload;
	else
		priv->irq_counter--;

	if (priv->irq_counter == 0 && priv->irq_enable)
	{
		priv->irq_active = true;
	}
}

bool mmc3_irq(nes_rom_info* context)
{
    mmc3_t* priv = GET_SELF(mmc3_t, context->mapper);
    if (priv->irq_active)
	{
		priv->irq_active = false;
		return true;
	}
    return false;
}

void mmc3_init(nes_rom_info* context, mapper_t* mapper)
{
    mapper->priv = malloc(sizeof(mmc3_t));
    memset(mapper->priv, 0x00, sizeof(mmc3_t));
    mmc3_t* priv = (mmc3_t*)mapper->priv;
    priv->prg_banks[0] = context->prg_area;
    priv->prg_banks[1] = context->prg_area + 0x2000;
    priv->prg_banks[2] = context->prg_area + 0x2000 * 0x1E;
    priv->prg_banks[3] = context->prg_area + 0x2000 * 0x1F;
    for (int i = 0; i < 8; i++)
    {
        priv->chr_banks[i] = context->chr_area + i * 0x0400;
    }

    mapper->cpu_read = mmc3_cpu_read;
    mapper->cpu_write = mmc3_cpu_write;
    mapper->ppu_read = mmc3_ppu_read;
    mapper->get_mirror = mmc3_get_mirror;
    mapper->irq = mmc3_irq;
    mapper->scanline_handler = mmc3_scanline_handler;
}
