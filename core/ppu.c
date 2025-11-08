#include "ppu.h"

bool cart_ppu_mapped_addr(u16) { return false; }
u8   cart_ppu_read(u16 addr) { return 0x00; }
void cart_ppu_write(u16 addr, u8 data) { }

#pragma region "registers"
typedef union {
struct {
    u8 unused          : 5;
    u8 sprite_overflow : 1;
    u8 sprite_hit_zero : 1;
    u8 vertical_blank  : 1;
};
    u8 reg;
} status_register;

typedef union {
struct {
    u8 grayscale                : 1;
    u8 render_background_left   : 1;
    u8 render_sprites_left      : 1;
    u8 render_background        : 1;
    u8 render_sprites           : 1;
    u8 enhance_red              : 1;
    u8 enhance_green            : 1;
    u8 enhance_blue             : 1;
};
    u8 reg;
} mask_regsiter;

typedef union {
struct {
    u8 nametable_x          : 1;
    u8 nametable_y          : 1;
    u8 increment_mode       : 1;
    u8 pattern_sprite       : 1;
    u8 pattern_background   : 1;
    u8 sprite_size          : 1;
    u8 slave_mode           : 1; // unused
    u8 enable_nmi           : 1;
};
    u8 reg;
} control_register;

typedef union  {
struct {
    u16 coarse_x            : 5;
    u16 coarse_y            : 5;
    u16 nametable_x         : 1;
    u16 nametable_y         : 1;
    u16 fine_y              : 3;
    u16 unused              : 1;
};
    u16 reg;
} loopy_address_register;

static mask_regsiter            __mask;
static status_register          __status;
static control_register         __control;
static loopy_address_register   __vram_addr;
static loopy_address_register   __tram_addr;


#pragma endregion


#pragma region "ppu bus"

enum ppu_logic_address_space
{
    PATTERN_TABLE_0_BOUND       =   0x1000,
    PATTERN_TABLE_1_BOUND       =   0x2000,
    NAME_TABLE_0_BOUND          =   0x23C0,
    ATTRIB_TABLE_0_BOUND        =   0x2400,
    NAME_TABLE_1_BOUND          =   0x27C0,
    ATTRIB_TABLE_1_BOUND        =   0x2800,
    NAME_TABLE_2_BOUND          =   0x2BC0,
    ATTRIB_TABLE_2_BOUND        =   0x2C00,
    NAME_TABLE_3_BOUND          =   0x2FC0,
    ATTRIB_TABLE_3_BOUND        =   0x3000,
    UNUSED_BOUND                =   0x3F00,
    PAL_RAM_BOUND               =   0x3F20,
    PAL_RAM_MIRROR_BOUND        =   0x4000,
};

enum ppu_io_device
{
    UNKOWN,
    CART,
    VRAM,       // video ram, including data of name-table and attribe-table
    PRAM,       // palatte ram, including data of palatte
};

static u8 __data_buffer = 0x00;
static u8 __latch_address = 0; 
static u8 __vram[0x800];

static u16 mirror_vram_addr(u16 addr) { return addr & 0x07FF; }
static u8 ppu_read_vram(u16 addr)
{
    return __vram[mirror_vram_addr(addr & 0x0FFF)];
}

static u8 ppu_read_pram(u16 addr)
{
    return 0x00;
}

static void ppu_write_vram(u16 addr, u8 data)
{
    __vram[mirror_vram_addr(addr & 0x0FFF)] = data;
}

static void ppu_write_pram(u16 addr, u8 data)
{

}

static int ppu_bus_memory_map(u16 addr)
{
    if (addr < UNUSED_BOUND && cart_ppu_mapped_addr(addr))           
        return CART;

    else if (addr < PATTERN_TABLE_1_BOUND)  return CART;
    else if (addr < ATTRIB_TABLE_3_BOUND)   return VRAM;
    else if (addr < UNUSED_BOUND)           return UNKOWN;
    else                                    return PRAM;
}

static u8 ppu_internal_read(u16 addr)
{
    switch (ppu_bus_memory_map(addr))
    {
    case CART: return cart_ppu_read(addr);
    case VRAM: return ppu_read_vram(addr);
    case PRAM: return ppu_read_pram(addr);
        break;
    default:
        break;
    }
}

static void ppu_internal_write(u16 addr, u8 data)
{
    switch (ppu_bus_memory_map(addr))
    {
    case CART: cart_ppu_write(addr, data); return;
    case VRAM: ppu_write_vram(addr, data); return;
    case PRAM: ppu_write_pram(addr, data); return;
    default:
        break;
    }
}


#pragma endregion


#pragma region "cpu bus io"
enum cpu_bus_io
{
    PPU_CTRL       = 0x0000,
    PPU_MASK       = 0x0001,
    PPU_STATUS     = 0x0002,
    OAM_ADDR       = 0x0003,
    OAM_DATA       = 0x0004,
    PPU_SCROLL     = 0x0005,
    PPU_ADDR       = 0x0006,
    PPU_DATA       = 0x0007,
    PPU_IO_END
};

u8   ppu_bus_read(u16 addr)
{
    u8 data = 0x00;
    switch (addr & 0x0007)
    {
    case PPU_STATUS: {
        data = (__status.reg & 0xE0) | (__data_buffer & 0x1F);
        __status.vertical_blank = 0;
        break;
    }
    case OAM_DATA: { break; }
    case PPU_DATA: {
        data = __data_buffer;
        __data_buffer = ppu_internal_read(__vram_addr.reg);
        if (__vram_addr.reg >= 0x3F00) {
            data = __data_buffer;
        }
        __vram_addr.reg += (__control.increment_mode ? 32 : 1);
        break;
    }
    default:
        break;
    }
    return data;
}

void ppu_bus_write(u16 addr, u8 data)
{
    switch (addr & 0x0007)
    {
    case PPU_CTRL: {
        __control.reg = data;
        __tram_addr.nametable_x = __control.nametable_x;
        __tram_addr.nametable_y = __control.nametable_y;
        break;
    }
    case PPU_MASK: __mask.reg = data; break;
    case OAM_ADDR: break;
    case OAM_DATA: break;
    case PPU_SCROLL: break;
    case PPU_ADDR: {
        if (__latch_address == 0) {
            __tram_addr.reg = ((data & 0x3F) << 8) | (__tram_addr.reg & 0x00FF);
            __latch_address = 1; 
        } else {
            __tram_addr.reg = data | (__tram_addr.reg & 0xFF00);
            __vram_addr = __tram_addr;
            __latch_address = 0;
        }
    }
    case PPU_DATA:{
        ppu_internal_write(__vram_addr.reg, data);
        __vram_addr.reg += (__control.increment_mode ? 32 : 1);
        break;
    }
    default:
        break;
    }
}

#pragma endregion

#pragma region "basics"
static bool __nmi = false;
static bool __frame_complete = false;
static u16 __frame_buffer[256*240];

void ppu_clock()
{
    // test
    static int scanline = 0;
    static int cycle = 0;

    if (scanline == 241 && cycle == 1) {
        __status.vertical_blank = 1;
        if (__control.enable_nmi)
            __nmi = true;
    }

    cycle++;
    if (cycle == 341) {
        cycle = 0;
        scanline++;
        if (scanline == 262) {
            scanline = 0;
        } else if (scanline == 261) {
            goto LAST_CYCLE_OF_FRAME;
        }
    }
    return;
LAST_CYCLE_OF_FRAME:
    if (__mask.render_background) {
        // for test, render a frame with name-table in one clock
        for (int i = 0; i < 240; ++i) 
        {
            for (int j = 0; j < 256; ++j) 
            {
                u8 name = (ppu_internal_read(0x2000 | (((i >> 3) << 5) | (j >> 3))));
                {
                    // 提取RGB332的各分量
                    uint8_t r3 = (name >> 5) & 0x07;  // 高3位R（0~7）
                    uint8_t g3 = (name >> 2) & 0x07;  // 中间3位G（0~7）
                    uint8_t b2 = name & 0x03;         // 低2位B（0~3）
                    
                    // 扩展为RGB565各分量
                    uint8_t r5 = (r3 << 2) | (r3 >> 1);  // 3位→5位：左移2位 + 填充最高位的1位（r3>>1取最高位）
                    uint8_t g5 = (g3 << 3) | (g3);       // 3位→6位：左移3位 + 填充最高位的3位（g3本身重复）
                    uint8_t b5 = (b2 << 3) | (b2 << 1) | (b2 >> 1);  // 2位→5位：左移3位 + 填充最高位的2位
                    
                    // 组合为16位RGB565（高5位R，中6位G，低5位B）
                    __frame_buffer[i * 256 + j] = (u16)((r5 << 11) | (g5 << 5) | b5);
                }
            }
        }
        __frame_complete = true;
    }
}

bool ppu_nmi_triggered()
{
    if (__nmi) {
        __nmi = false;
        return true;
    }
    return false;
}

u16* ppu_frame_buffer()
{
    if (__frame_complete) {
        __frame_complete = false;
        return __frame_buffer;
    }
    return NULL;
}

#pragma endregion