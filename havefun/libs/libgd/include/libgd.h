
// libgd.h
// graphics device library.

#ifndef __LIBGD_H
#define __LIBGD_H  1

#include "vk.h"       // # view input events
#include "lt8x8.h"


int libgd_initialize(void);

int 
grBackBufferPutpixel ( 
    unsigned int color, 
    int x, 
    int y,
    unsigned long rop );

int 
grBackBufferPutpixel2 ( 
    unsigned int color, 
    int x, 
    int y );

// put pixel
// low level.
int 
fb_BackBufferPutpixel ( 
    unsigned int color, 
    int x, 
    int y,
    unsigned long rop,
    unsigned long buffer_va );

void 
putpixel0 ( 
    unsigned int  _color,
    unsigned long _x, 
    unsigned long _y, 
    unsigned long _rop_flags,
    unsigned long buffer_va );

void 
backbuffer_putpixel ( 
    unsigned int  _color,
    unsigned long _x, 
    unsigned long _y, 
    unsigned long _rop_flags );

void 
frontbuffer_putpixel ( 
    unsigned int  _color,
    unsigned long _x, 
    unsigned long _y, 
    unsigned long _rop_flags );

unsigned int grBackBufferGetPixelColor( int x, int y );

#endif    



