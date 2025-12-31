#include "../mapper.h"

u8 nrom_cpu_read(nes_rom_info* context, u16 addr)
{
    if (addr < MAPPER_BASE) 
        return 0x00;
    
    if (context->prg_rom_pages == 1) {
        return context->prg_area[(addr - MAPPER_BASE) & 0x3FFF];
    }
    return context->prg_area[(addr - MAPPER_BASE)];
}

u8 nrom_ppu_read(nes_rom_info* context, u16 addr) 
{
    return context->chr_area[addr & (CHR_BANK_SIZE - 1)];
}

void nrom_init(mapper_t* mapper)
{
    mapper->cpu_read = nrom_cpu_read;
    mapper->ppu_read = nrom_ppu_read;
}
