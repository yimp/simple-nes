#ifndef __CART_H__
#define __CART_H__
#include "types.h"

enum
{ 
    CartMirrorHorizontal, 
    CartMirrorVertical, 
    CartMirrorFourScreen 
}; 

void cart_load(const u8* data);
u8   cart_read(u16 addr);
void cart_write(u16 addr, u8 data);

#endif