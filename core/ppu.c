#include "ppu.h"
#include "cart.h"
#include <string.h>

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
static u8                       __fine_x;

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

static u16 mirror_vram_addr(u16 addr) 
{ 
    addr &= 0x0FFF;
    switch (cart_ppu_get_mirror()) 
	{
	case CartMirrorHorizontal: // NT0,NT1 → vram[0x000~0x3FF], NT2,NT3 → vram[0x400~0x7FF]
		return (((addr >> 10) >> 1) << 10) | (addr & 0x3FF);
	case CartMirrorVertical: // NT0,NT2 → vram[0x000~0x3FF], NT1,NT3 → vram[0x400~0x7FF]
		return (((addr >> 10) & 1) << 10) | (addr & 0x3FF);
	default: 
        break;
    }
    return 0;
}

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

#pragma region "OAM"
typedef union
{
struct
{
	u8 y;			// Y position of sprite
	u8 id;			// ID of tile from pattern memory
	u8 attribute;	// Flags define how sprite should be rendered
	u8 x;			// X position of sprite
};
u8 bytes[4];
}  sprite;
static sprite __oam[64];
static u8 __oam_addr = 0x00;
static sprite __2nd_oam[8];

void ppu_dma_oam_write(u8 addr, u8 data)
{
    ((u8*)__oam)[addr] = data;
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
    case OAM_DATA: {
        u8 *ptr = (u8*)__oam;
        data = ptr[__oam_addr];
        break; 
    }
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
    case OAM_ADDR: __oam_addr = data; break;
    case OAM_DATA: {
        u8 *ptr = (u8*)__oam;
        ptr[__oam_addr] = data;
        break;
    }
    case PPU_SCROLL: {
        if (__latch_address == 0) {
			__fine_x = data & 0x07;
			__tram_addr.coarse_x = data >> 3;
			__latch_address = 1;
		}
		else {
			__tram_addr.fine_y = data & 0x07;
			__tram_addr.coarse_y = data >> 3;
			__latch_address = 0;
		}
		break;
    }
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

// #pragma region "basics"
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


static inline u16 fg_8x8_tile_lsb_addr(u8 id, u8 y, u8 attr, int scanline)
{
	u8 t = (0x08 - ((attr & 0x80) >> 7)) & 0x07; // t is 0b000 or 0b111
	return __control.pattern_sprite << 12 | id << 4 | (t ^ (scanline - y));
}
static inline u16 fg_8x8_tile_msb_addr(u8 id, u8 y, u8 attr,int scanline)
{
	u8 t = (0x08 - ((attr & 0x80) >> 7)) & 0x07; // t is 0b000 or 0b111
	return __control.pattern_sprite << 12 | id << 4 | 0b1000 | (t ^ (scanline - y));
}

static inline u16 fg_8x16_tile_lsb_addr(u8 id, u8 y, u8 attr, int scanline)
{
	u8 t = (0x10 - ((attr & 0x80) >> 7)) & 0x0F; // t is 0b0000 or 0b1111
	u8 d = (t ^ ((scanline - y) & 0x0F)); // 0x00 - 0x0F
	return ((id & 0x01) << 12) | (((id & 0xFE) | ((d >> 3) & 1)) << 4) | (d & 0x07);
}
static inline u16 fg_8x16_tile_msb_addr(u8 id, u8 y, u8 attr, int scanline)
{
	u8 t = (0x10 - ((attr & 0x80) >> 7)) & 0x0F; // t is 0b0000 or 0b1111
	u8 d = (t ^ ((scanline - y) & 0x0F)); // 0x00 - 0x0F
	return ((id & 0x01) << 12) | (((id & 0xFE) | ((d >> 3) & 1)) << 4) | 0b1000 | (d & 0x07);
}

static u8 fg_tile_hflip(u8 bits) 
{
    bits = (bits & 0b11110000) >> 4 | (bits & 0b00001111) << 4;
    bits = (bits & 0b11001100) >> 2 | (bits & 0b00110011) << 2;
    bits = (bits & 0b10101010) >> 1 | (bits & 0b01010101) << 1;
    return bits;
}

static inline bool sprite_over_scanline(int scanline, i16 y) 
{
    i16 diff = ((i16)scanline - y);
    return (diff >= 0 && diff < (__control.sprite_size ? 16 : 8));
}

static u16 __bg_tile_lo_shifter    = 0x0000;
static u16 __bg_tile_hi_shifter    = 0x0000;
static u16 __bg_attr_lo_shifter    = 0x0000;
static u16 __bg_attr_hi_shifter    = 0x0000;

static u8  __fg_tile_lsb[8];
static u8  __fg_tile_msb[8];
static u8  __fg_tile_x_latch[8];
static u8  __fg_tile_attr_latch[8];
static u8  __fg_tile_lsb_shifters[8];
static u8  __fg_tile_msb_shifters[8];

static void bg_fetch(int cycle)
{
    static u8 bg_name_latch = 0x00;
    static u8 bg_attr_latch = 0x00;
    static u8 bg_tile_lsb_latch = 0x00;
    static u8 bg_tile_msb_latch = 0x00;
    switch (cycle & 7)
    {
    case 1: // cycle 1, 2: fetch data from nametable
        bg_name_latch =  (ppu_internal_read(0x2000 | (__vram_addr.reg & 0x0FFF)));
        break;
    case 3: // cycle 3, 4: fetch data from attribute-table
        bg_attr_latch = ppu_internal_read(attrib_addr());
        bg_attr_latch = attrib_component(bg_attr_latch);
        break;
    case 5: // cycle 5, 6: fetch pattern data from chr
        bg_tile_lsb_latch =  ppu_internal_read(bg_tile_lsb_addr(bg_name_latch));
        break;
    case 7: // cycle 7, 8: fetch pattern data from chr
        bg_tile_msb_latch =  ppu_internal_read(bg_tile_msb_addr(bg_name_latch));
        break;
    case 0: // cycle 8, 16, 24...: load data in latches to shift registers
        __bg_tile_lo_shifter = (__bg_tile_lo_shifter & 0xFF00) | bg_tile_lsb_latch;
        __bg_tile_hi_shifter = (__bg_tile_hi_shifter & 0xFF00) | bg_tile_msb_latch;

        __bg_attr_lo_shifter = (__bg_attr_lo_shifter & 0xFF00) | ((bg_attr_latch & 0x01) ? 0xFF : 0x00);
        __bg_attr_hi_shifter = (__bg_attr_hi_shifter & 0xFF00) | ((bg_attr_latch & 0x02) ? 0xFF : 0x00);
    default:
        break;
    }
}

static void bg_shift()
{
    if (__mask.render_background) 
    {
        __bg_attr_lo_shifter <<= 1;
        __bg_attr_hi_shifter <<= 1;
        __bg_tile_lo_shifter <<= 1;
        __bg_tile_hi_shifter <<= 1;
    }
}

static void nt_move_x()
{
	if (__mask.render_background || __mask.render_sprites)
	{
		if (__vram_addr.coarse_x == 31)
		{
			__vram_addr.coarse_x = 0;
			__vram_addr.nametable_x = ~__vram_addr.nametable_x;
		}
		else
		{
			__vram_addr.coarse_x++;
		}
	}    
}

static void nt_move_y()
{
    if (__mask.render_background || __mask.render_sprites)
    {
        if (__vram_addr.fine_y < 7)
        {
            __vram_addr.fine_y++;
        }
        else
        {
            __vram_addr.fine_y = 0;
            if (__vram_addr.coarse_y == 29)
            {
                __vram_addr.coarse_y = 0;
                __vram_addr.nametable_y = ~__vram_addr.nametable_y;
            }
            else if (__vram_addr.coarse_y == 31)
            {
                __vram_addr.coarse_y = 0;
            }
            else
            {
                __vram_addr.coarse_y++;
            }
        }
    }
}

static void nt_copy_x(void)
{
    if (__mask.render_background || __mask.render_sprites)
    {
        __vram_addr.nametable_x = __tram_addr.nametable_x;
        __vram_addr.coarse_x    = __tram_addr.coarse_x;
    }
};

static void nt_copy_y(void)
{
    if (__mask.render_background || __mask.render_sprites)
    {
        __vram_addr.fine_y      = __tram_addr.fine_y;
        __vram_addr.nametable_y = __tram_addr.nametable_y;
        __vram_addr.coarse_y    = __tram_addr.coarse_y;
    }
};


u8 bg_pixel()
{
    u8 bg_pixel = 0x00;
    u8 bg_palette = 0x00;
    u16 bit_mux = 0x8000 >> __fine_x;
    bg_pixel   = ((((__bg_tile_hi_shifter & bit_mux) > 0) << 1) | ((__bg_tile_lo_shifter & bit_mux) > 0));
    bg_palette = ((((__bg_attr_hi_shifter & bit_mux) > 0) << 1) | ((__bg_attr_lo_shifter & bit_mux) > 0));
    return (bg_palette << 2) | bg_pixel;
}


static void sprite_evaluation_step(int scanline, int cycle)
{
    static u8 n = 0;
    static u8 m = 0;
    static u8 offset = 0;
    if (cycle == 65)
    {
        memset(__2nd_oam, 0xFF, sizeof(__2nd_oam));
        m = 0; n = 0; offset = 0;
    }

    if (n >= 64) return;

    if ((cycle & 1) == 1 && offset == 0) {
        if (!sprite_over_scanline(scanline, __oam[n].y))
        {
            n++;
            offset = 0;
            return;
        }

        if (m < 8)
        {
            __2nd_oam[m].bytes[offset] = __oam[n].y;
        }

        offset++;
    }

    if ((cycle & 1) == 1 && offset > 0) {
        if (m < 8) {
            __2nd_oam[m].bytes[offset] = __oam[n].bytes[offset];
            offset = (offset + 1) & 0x03;
            m += (offset == 0);
            n += (offset == 0);
        }
        else {
            __status.sprite_overflow = 1;
            n++;
        }
    }
}

static void fg_fetch(int scanline, int cycle)
{
    static u8 n = 0;
    static u8 fg_tile_lsb_latch = 0x00;
    static u8 fg_tile_msb_latch = 0x00;
    switch (cycle & 7)
    {
    case 1: // cycle 1, 2
        break;
    case 3: // attr
        __fg_tile_attr_latch[n] = __2nd_oam[n].attribute;
        break;
    case 4:
        __fg_tile_x_latch[n] = __2nd_oam[n].x;
        break;
    case 5: // pattern lsb
        fg_tile_lsb_latch = ppu_internal_read(__control.sprite_size ? 
            fg_8x16_tile_lsb_addr(__2nd_oam[n].id, __2nd_oam[n].y, __2nd_oam[n].attribute, scanline)
            : fg_8x8_tile_lsb_addr(__2nd_oam[n].id, __2nd_oam[n].y, __2nd_oam[n].attribute, scanline));
        if (__2nd_oam[n].y == 0xFF)
            fg_tile_lsb_latch = 0x00;
        break;
    case 7: // pattern msb
        fg_tile_msb_latch = ppu_internal_read(__control.sprite_size ? 
            fg_8x16_tile_msb_addr(__2nd_oam[n].id, __2nd_oam[n].y, __2nd_oam[n].attribute, scanline)
            : fg_8x8_tile_msb_addr(__2nd_oam[n].id, __2nd_oam[n].y, __2nd_oam[n].attribute, scanline));
        if (__2nd_oam[n].y == 0xFF)
            fg_tile_msb_latch = 0x00;
        break;
    case 0: // load shifters
        __fg_tile_lsb_shifters[n] = (__fg_tile_attr_latch[n] & 0x40) ? 
            fg_tile_hflip(fg_tile_lsb_latch) : fg_tile_lsb_latch;

        __fg_tile_msb_shifters[n] = (__fg_tile_attr_latch[n] & 0x40) ? 
            fg_tile_hflip(fg_tile_msb_latch) : fg_tile_msb_latch;
        n = (n + 1) & 0x07;
    default:
        break;
    }
}

static void fg_shift(void)
{
    if (__mask.render_sprites)
    {
        for (int i = 0; i < 8; i++)
        {
            if (__fg_tile_x_latch[i] > 0)
                --__fg_tile_x_latch[i];
            else {
                (__fg_tile_lsb_shifters[i]) <<= 1;
                (__fg_tile_msb_shifters[i]) <<= 1;
            }
        }
    }
}

u8 fg_pixel()
{
    u8 fg_pixel = 0x00;
    u8 fg_palette = 0x00;
    u8 fg_priority = 0x00;

    u8 i = 0;
    for (; i < 8; ++i) {
        if (__fg_tile_x_latch[i]) {
            continue;
        }

        fg_pixel = ((__fg_tile_msb_shifters[i] & 0x80) >> 7 << 1) | ((__fg_tile_lsb_shifters[i] & 0x80) >> 7);
        fg_palette = __fg_tile_attr_latch[i] & 0x03;
        fg_priority = (__fg_tile_attr_latch[i] & 20) >> 5;
        if (fg_pixel != 0x00) {
            break;
        }
    }
	return ((i & 0x07) << 5) | ((fg_priority & 1) << 4) | (fg_palette & 0x03) << 2 | (fg_pixel & 0x03);
}

#pragma endregion

void ppu_clock()
{
    static int scanline = 0;
    static int cycle = 0;
    static bool sprite_zero_over_scanline = false;

    if ((scanline == 261) || scanline >= 0 && scanline <= 239) 
    {
        if (cycle >= 1 && cycle <= 256 || cycle >= 321 && cycle <= 336)
        {
            bg_shift();
            bg_fetch(cycle);
            if ((cycle & 7) == 0)
                nt_move_x();
            if (cycle == 256)
                nt_move_y();
        }

        if (cycle == 257) // reset x
            nt_copy_x();
        if (scanline == 261 && cycle >= 280 && cycle < 305)
            nt_copy_y();

        if (scanline <= 239 && cycle >= 65 && cycle <= 256)
        {
            sprite_evaluation_step(scanline, cycle);
            if (cycle == 256)
                sprite_zero_over_scanline = __2nd_oam[0].y == __oam[0].y;
        }

        if (cycle >= 257 & cycle <= 329)
        {
            fg_fetch(scanline, cycle);
        }

        if (scanline == 261 && cycle == 1)
		{
			__status.vertical_blank = 0;
			__status.sprite_overflow = 0;
			__status.sprite_hit_zero = 0;
		}
        goto RENDER;
    }

    if (scanline == 241 && cycle == 1) {
        __status.vertical_blank = 1;
        if (__control.enable_nmi)
            __nmi = true;
		goto RENDER;
    }

    // idle scanline
    if (scanline == 240)
        goto RENDER;

RENDER:
    if (cycle < 256 && scanline < 240) 
    {
        u8 bg_pixel_data = 0x00;
        u8 fg_pixel_data = 0x00;

        if (__mask.render_background && (__mask.render_background_left || (cycle >= 9))) {
            bg_pixel_data = bg_pixel();
        }

        if (__mask.render_sprites && (__mask.render_sprites_left || (cycle >= 9))) {
            fg_pixel_data = fg_pixel();
        }

        if ((fg_pixel_data & 0x03) == 0 || ((fg_pixel_data & 0x10) > 0 && ((bg_pixel_data & 0x03) != 0)))
            draw_one_pixel(cycle, scanline, ppu_internal_read(0x3F00 | (bg_pixel_data & 0x0F)));
        else 
            draw_one_pixel(cycle, scanline, ppu_internal_read(0x3F10 | (fg_pixel_data & 0x0F)));

        if (sprite_zero_over_scanline && (fg_pixel_data & 0x03 > 0) && (fg_pixel_data & 0xE0) == 0) {
            __status.sprite_hit_zero = 1;
        }
        fg_shift();
    }

    cycle++;
    if (cycle == 341) {
        cycle = 0;
        scanline++;
        if (scanline == 262) {
            scanline = 0;
        } else if (scanline == 261) {
            __frame_complete = true;
        }
    }
    return;
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
