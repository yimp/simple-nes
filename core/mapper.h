#ifndef __MAPPER_H__
#define __MAPPER_H__
#include "types.h"

#define TRAINER_SIZE 512
#define PRG_BANK_SIZE 16384
#define CHR_BANK_SIZE 8192
#define MAPPER_BASE 0x8000

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

    const u8* trainer_area;
    const u8* prg_area ;
    const u8* chr_area;
    u8* battery_ram;

    // mapper
    void* mapper;
} nes_rom_info;

typedef struct {
    // CPU io
    u8      (*cpu_read)(nes_rom_info*, u16);
    void    (*cpu_write)(nes_rom_info*, u16, u8);

    // PPU io
    u8      (*ppu_read)(nes_rom_info*, u16);
    void    (*ppu_write)(nes_rom_info*, u16, u8);

    // control
    bool    (*irq)(nes_rom_info*);
    void    (*scanline_handler)(nes_rom_info*);
    bool    (*ppu_mapped_addr)(nes_rom_info*, u16);
    u8      (*get_mirror)(nes_rom_info*);

    // private data
    void* priv;
} mapper_t;

void mapper_init(nes_rom_info* context, mapper_t* mapper);
void mapper_destroy(nes_rom_info* context, mapper_t** pmapper);
#define GET_SELF(type, ptr) ((type*)(((mapper_t*)(ptr))->priv))

#endif