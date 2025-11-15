#include "ppu.h"
#include "cart.h"


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
static u8 __pram[0x020];

static u16 mirror_vram_addr(u16 addr) { return addr & 0x07FF; }
static u8 ppu_read_vram(u16 addr)
{
    return __vram[mirror_vram_addr(addr & 0x0FFF)];
}

static u8 ppu_read_pram(u16 addr)
{
    // Palette RAM: $3F00-$3FFF, mirrored every 32 bytes
    u16 pal_addr = (addr - 0x3F00) & 0x1F;
    if ((pal_addr & 0x03) == 0) 
        pal_addr = 0x00; // transparent
    return __pram[pal_addr];
}

static void ppu_write_vram(u16 addr, u8 data)
{
    __vram[mirror_vram_addr(addr & 0x0FFF)] = data;
}

static void ppu_write_pram(u16 addr, u8 data)
{
    // Palette RAM: $3F00-$3FFF, mirrored every 32 bytes
    u16 pal_addr = (addr - 0x3F00) & 0x1F;
    if (pal_addr == 0x10) pal_addr = 0x00;
    if (pal_addr == 0x14) pal_addr = 0x04;
    if (pal_addr == 0x18) pal_addr = 0x08;
    if (pal_addr == 0x1C) pal_addr = 0x0C;
    __pram[pal_addr] = data;
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
        break;
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
#define PIXEL_COLOR_RGB_565(r, g, b) \
    (u16)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3))
#define COLOR(r, g, b) PIXEL_COLOR_RGB_565(r, g, b)
static bool __nmi = false;
static bool __frame_complete = false;
static u16 __frame_buffer[256*240];
static u16 __system_palette[0x40] = {
    COLOR( 84,  84,  84), COLOR(  0,  30, 116), COLOR(  8,  16, 144), COLOR( 48,   0, 136),
	COLOR( 68,   0, 100), COLOR( 92,   0,  48), COLOR( 84,   4,   0), COLOR( 60,  24,   0),
	COLOR( 32,  42,   0), COLOR(  8,  58,   0), COLOR(  0,  64,   0), COLOR(  0,  60,   0),
	COLOR(  0,  50,  60), COLOR(  0,   0,   0), COLOR(  0,   0,   0), COLOR(  0,   0,   0),
	COLOR(152, 150, 152), COLOR(  8,  76, 196), COLOR( 48,  50, 236), COLOR( 92,  30, 228),
	COLOR(136,  20, 176), COLOR(160,  20, 100), COLOR(152,  34,  32), COLOR(120,  60,   0),
	COLOR( 84,  90,   0), COLOR( 40, 114,   0), COLOR(  8, 124,   0), COLOR(  0, 118,  40),
	COLOR(  0, 102, 120), COLOR(  0,   0,   0), COLOR(  0,   0,   0), COLOR(  0,   0,   0),
	COLOR(236, 238, 236), COLOR( 76, 154, 236), COLOR(120, 124, 236), COLOR(176,  98, 236),
	COLOR(228,  84, 236), COLOR(236,  88, 180), COLOR(236, 106, 100), COLOR(212, 136,  32),
	COLOR(160, 170,   0), COLOR(116, 196,   0), COLOR( 76, 208,  32), COLOR( 56, 204, 108),
	COLOR( 56, 180, 204), COLOR( 60,  60,  60), COLOR(  0,   0,   0), COLOR(  0,   0,   0),
	COLOR(236, 238, 236), COLOR(168, 204, 236), COLOR(188, 188, 236), COLOR(212, 178, 236),
	COLOR(236, 174, 236), COLOR(236, 174, 212), COLOR(236, 180, 176), COLOR(228, 196, 144),
	COLOR(204, 210, 120), COLOR(180, 222, 120), COLOR(168, 226, 144), COLOR(152, 226, 180),
	COLOR(160, 214, 228), COLOR(160, 162, 160), COLOR(  0,   0,   0), COLOR(  0,   0,   0),
};

#pragma region "render"
#define NAME_TABLE_0_BASE		0x2000
#define ATTRIB_TABLE_0_BASE		0x23C0
#define NAME_TABLE_1_BASE		0x2400
#define ATTRIB_TABLE_1_BASE		0x27C0
#define NAME_TABLE_2_BASE		0x2800
#define ATTRIB_TABLE_2_BASE		0x23C0
#define NAME_TABLE_3_BASE		0x2C00
#define ATTRIB_TABLE_3_BASE		0x2FC0

static inline u16 attrib_addr()
{
    return ATTRIB_TABLE_0_BASE
        | ( __vram_addr.nametable_y << 11)
        | ( __vram_addr.nametable_x << 10)
        | ((__vram_addr.coarse_y >> 2) << 3)
        | ( __vram_addr.coarse_x >> 2);
}

static inline u8 attrib_component(u8 attrib)
{
    return (attrib >> (((__vram_addr.coarse_y & 2) | ((__vram_addr.coarse_x & 2) >> 1)) << 1)) & 0x03;
}

static inline u16 bg_tile_lsb_addr(u8 name)
{
    return __control.pattern_background << 12
         | name << 4
         | __vram_addr.fine_y;
}

static inline u16 bg_tile_msb_addr(u8 name)
{
    return __control.pattern_background << 12
         | name << 4
         | 0b1000
         | __vram_addr.fine_y;
}

static inline void draw_one_pixel(i16 x, i16 y, u8 palette_index)
{
    __frame_buffer[y * 256 + x] = __system_palette[palette_index & 0x3F];
}

#pragma endregion

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
                __vram_addr.reg = (((i >> 3) << 5) | (j >> 3));
                __vram_addr.fine_y = (i & 0x07);

                u8 name = (ppu_internal_read(0x2000 | (__vram_addr.reg & 0x0FFF)));
                u8 attr = ppu_internal_read(attrib_addr());
                attr = attrib_component(attr);

                u8 bg_tile_lsb = ppu_internal_read(bg_tile_lsb_addr(name));
                u8 bg_tile_msb = ppu_internal_read(bg_tile_msb_addr(name));

                u8 bg_attr_lsb = ((attr & 0x01) ? 0xFF : 0x00); // attr 10 -> FF 00
                u8 bg_attr_msb = ((attr & 0x02) ? 0xFF : 0x00);

                {
                    u8 lo = (bg_tile_lsb << (j & 0x07)) & 0x80;
                    u8 hi = (bg_tile_msb << (j & 0x07)) & 0x80;
                    u8 bg_pixel = ((hi > 0) << 1) | (lo > 0);

                    lo = (bg_attr_lsb << (j & 0x07)) & 0x80;
                    hi = (bg_attr_msb << (j & 0x07)) & 0x80;
                    u8 bg_palette = ((hi > 0) << 1) | (lo > 0); // bg_palette == attr

                    u8 bg_pixel_data = ((bg_palette & 0x03) << 2) | (bg_pixel & 0x03);
                    draw_one_pixel(j, i, ppu_internal_read(0x3F00 | (bg_pixel_data)));
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