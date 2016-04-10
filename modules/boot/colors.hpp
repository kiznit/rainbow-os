/*
    Copyright (c) 2016, Thierry Tremblay
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _RAINBOW_BOOT_COLORS_HPP
#define _RAINBOW_BOOT_COLORS_HPP

#include <rainbow/types.h>


// All color constants are in the sRGB color space

#define COLOR_BLACK             0x000000


// VGA text mode color palette
//
// Source:
//  CGA: https://en.wikipedia.org/wiki/Color_Graphics_Adapter
//  EGA: https://en.wikipedia.org/wiki/Enhanced_Graphics_Adapter
#define COLOR_VGA_BLACK         0x000000
#define COLOR_VGA_BLUE          0x0000aa
#define COLOR_VGA_GREEN         0x00aa00
#define COLOR_VGA_CYAN          0x00aaaa
#define COLOR_VGA_RED           0xaa0000
#define COLOR_VGA_MAGENTA       0xaa00aa
#define COLOR_VGA_BROWN         0xaa5500
#define COLOR_VGA_LIGHT_GRAY    0xaaaaaa
#define COLOR_VGA_DARK_GRAY     0x555555
#define COLOR_VGA_LIGHT_BLUE    0x5555ff
#define COLOR_VGA_LIGHT_GREEN   0x55ff55
#define COLOR_VGA_LIGHT_CYAN    0x55ffff
#define COLOR_VGA_LIGHT_RED     0xff5555
#define COLOR_VGA_LIGHT_MAGENTA 0xff55ff
#define COLOR_VGA_YELLOW        0xffff55
#define COLOR_VGA_WHITE         0xffffff

// Rainbow colors
//
// Source:
//  https://simple.wikipedia.org/wiki/Rainbow

#define COLOR_RAINBOW_RED       0xff0000
#define COLOR_RAINBOW_ORANGE    0xff7f00
#define COLOR_RAINBOW_YELLOW    0xffff00
#define COLOR_RAINBOW_GREEN     0x00ff00
#define COLOR_RAINBOW_BLUE      0x0000ff
#define COLOR_RAINBOW_INDIGO    0x4b0082
#define COLOR_RAINBOW_VIOLET    0x8b00ff


// Find the closest color in a palette
int FindNearestColor(uint32_t color, const uint32_t* palette, int lengthPalette);




#endif
