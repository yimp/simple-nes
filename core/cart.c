#include "cart.h"
#include "mapper.h"
#include <assert.h>
#include <stdlib.h>

typedef struct {
    u8 magic[4]; // 'N' 'E' 'S' 0x1A
    u8 prg_rom_pages; // 16KB
    u8 chr_rom_pages; // 8KB
    u8 flags06;
    u8 flags07;
    u8 flags08;
    u8 flags09;
    u8 flags10;
    u8 flags11;
    u8 flags12;
    u8 reserved[3];
} nes_header;


static nes_rom_info __rom_info;

static void parse_nes_header(const u8* data)
{
    const nes_header* header = (const nes_header*)data;
    assert(header->magic[0] == 'N' && header->magic[1] == 'E' && header->magic[2] == 'S' && header->magic[3] == 0x1A);

    __rom_info.valid = true;
    __rom_info.is_nes2      = ((header->flags07 >> 2) & 0x03) == 0x02;
    __rom_info.battery      = header->flags06 & 0x02;
    __rom_info.has_trainer  = header->flags06 & 0x04;

    if (header->flags06 & 0x08)
    {
        __rom_info.mirroring = CartMirrorFourScreen;
    }
    else 
        __rom_info.mirroring = (header->flags06 & 0x01) == 0 ? CartMirrorHorizontal : CartMirrorVertical;

    u16 mapper_lo = (header->flags06 >> 4);
    u16 mapper_hi = (header->flags07 & 0xF0);
    __rom_info.mapper_id = mapper_hi | mapper_lo;

    if (!__rom_info.is_nes2) {
        __rom_info.chr_rom_pages = header->chr_rom_pages;
        __rom_info.prg_rom_pages = header->prg_rom_pages;
    }
}

void cart_load(const u8* data)
{
    parse_nes_header(data);

    int offset = sizeof(nes_header);
    if (__rom_info.has_trainer) {
       __rom_info.trainer_area = data + offset;
        offset += TRAINER_SIZE;
    }

    __rom_info.prg_area = data + offset;
    offset += __rom_info.prg_rom_pages * PRG_BANK_SIZE;

    __rom_info.chr_area = data + offset;
    offset += __rom_info.chr_rom_pages * CHR_BANK_SIZE;

    if (__rom_info.battery && !__rom_info.battery_ram) {
        __rom_info.battery_ram = (u8*)malloc(0x2000);
    }

    // TODD: destroy previous mapper if any
    __rom_info.mapper = (mapper_t*)malloc(sizeof(mapper_t));
    mapper_init(&__rom_info, __rom_info.mapper);
}

u8   cart_read(u16 addr)
{
    mapper_t* mapper = ((mapper_t*)(__rom_info.mapper));
    if (mapper->cpu_read) 
        return mapper->cpu_read(&__rom_info, addr);
    return 0x00;
}

void cart_write(u16 addr, u8 data)
{
    mapper_t* mapper = ((mapper_t*)(__rom_info.mapper));
    if (mapper->cpu_write) 
        mapper->cpu_write(&__rom_info, addr, data);
}

bool cart_ppu_mapped_addr(u16 addr) 
{ 
    mapper_t* mapper = ((mapper_t*)(__rom_info.mapper));
    if (mapper->ppu_mapped_addr) 
        return mapper->ppu_mapped_addr(&__rom_info, addr);
    return false;
}

u8   cart_ppu_read(u16 addr) 
{
    mapper_t* mapper = ((mapper_t*)(__rom_info.mapper));
    if (mapper->ppu_read) 
        return mapper->ppu_read(&__rom_info, addr);
    return 0x00;
}

void cart_ppu_write(u16 addr, u8 data) 
{
    mapper_t* mapper = ((mapper_t*)(__rom_info.mapper));
    if (mapper->ppu_write) 
        mapper->ppu_write(&__rom_info, addr, data);
}

u8   cart_ppu_get_mirror()
{
    mapper_t* mapper = ((mapper_t*)(__rom_info.mapper));
    if (mapper->get_mirror) 
        return mapper->get_mirror(&__rom_info);
    return __rom_info.mirroring;
}

bool cart_irq()
{
    mapper_t* mapper = ((mapper_t*)(__rom_info.mapper));
    if (mapper->irq) 
        return mapper->irq(&__rom_info);
    return false;
}

void cart_scanline_handler()
{
    mapper_t* mapper = ((mapper_t*)(__rom_info.mapper));
    if (mapper->scanline_handler) 
        mapper->scanline_handler(&__rom_info);
}
