/*
    Copyright (c) 2018, Thierry Tremblay
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

#ifndef RAINBOW_BOOT_GRAPHICS_VGACOLORS_HPP
#define RAINBOW_BOOT_GRAPHICS_VGACOLORS_HPP

#include <stdint.h>


// Foreground and background colors
enum Color
{
    Color_Black,        // 000000
    Color_Blue,         // 0000AA
    Color_Green,        // 00AA00
    Color_Cyan,         // 00AAAA
    Color_Red,          // AA0000
    Color_Magenta,      // AA00AA
    Color_Brown,        // AA5500
    Color_LightGray,    // AAAAAA
};


// Foreground only colors
enum ForegroundColor
{
    Color_DarkGray = 8, // 555555
    Color_LightBlue,    // 5555FF
    Color_LightGreen,   // 55FF55
    Color_LightCyan,    // 55FFFF
    Color_LightRed,     // FF5555
    Color_LightMagenta, // FF55FF
    Color_Yellow,       // FFFF55
    Color_White         // FFFFFF
};


// Find the closest VGA color index
int FindClosestVgaColor(uint32_t color, bool backgroundColor);



#endif
