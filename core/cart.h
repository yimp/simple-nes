#ifndef __CART_H__
#define __CART_H__
#include "types.h"

enum
{ 
    CartMirrorHorizontal, 
    CartMirrorVertical, 
    CartMirrorFourScreen 
}; 

// basics
void cart_load(const u8* data);

// cpu bus io
u8   cart_read(u16 addr);
void cart_write(u16 addr, u8 data);

// ppu bus io
bool cart_ppu_mapped_addr(u16);
u8   cart_ppu_read(u16 addr);
void cart_ppu_write(u16 addr, u8 data);
u8   cart_ppu_get_mirror();
#endif