#include "cart.h"
#include <assert.h>

#define TRAINER_SIZE 512
#define PRG_BANK_SIZE 16384
#define CHR_BANK_SIZE 8192
#define MAPPER_BASE 0x8000

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

typedef struct 
{
    bool valid;
    bool is_nes2;
    bool has_trainer;
    bool battery;

    u8  prg_rom_pages; // 16KB
    u8  chr_rom_pages; // 8KB
    u8  mirroring;
    u16 mapper_id;
} nes_rom_info;

static nes_rom_info __rom_info;
static const u8* __trainer_area = NULL;
static const u8* __prg_area = NULL ;
static const u8* __chr_area = NULL;

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
        __trainer_area = data + offset;
        offset += TRAINER_SIZE;
    }

    __prg_area = data + offset;
    offset += __rom_info.prg_rom_pages * PRG_BANK_SIZE;

    __chr_area = data + offset;
    offset += __rom_info.chr_rom_pages * CHR_BANK_SIZE;

    if (__rom_info.battery) { }
}

u8   cart_read(u16 addr)
{
    if (addr < MAPPER_BASE) 
        return 0x00;
    
    if (__rom_info.prg_rom_pages == 1) {
        return __prg_area[(addr - MAPPER_BASE) & 0x3FFF];
    }
    return __prg_area[(addr - MAPPER_BASE)];
}

void cart_write(u16 addr, u8 data)
{
}


bool cart_ppu_mapped_addr(u16) { return false; }
u8   cart_ppu_read(u16 addr) 
{
    return __chr_area[addr & (CHR_BANK_SIZE - 1)];
}

void cart_ppu_write(u16 addr, u8 data) { }