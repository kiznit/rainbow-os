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

#include "vgacolors.hpp"
#include <limits.h>



static const int vgaColorPalette[16][3] =
{
    0x00, 0x00, 0x00,   // Black
    0x00, 0x00, 0xAA,   // Blue
    0x00, 0xAA, 0x00,   // Green
    0x00, 0xAA, 0xAA,   // Cyan
    0xAA, 0x00, 0x00,   // Red
    0xAA, 0x00, 0xAA,   // Magenta
    0xAA, 0x55, 0x00,   // Brown
    0xAA, 0xAA, 0xAA,   // LightGray
    0x55, 0x55, 0x55,   // DarkGray
    0x55, 0x55, 0xFF,   // LightBlue
    0x55, 0xFF, 0x55,   // LightGreen
    0x55, 0xFF, 0xFF,   // LightCyan
    0xFF, 0x55, 0x55,   // LightRed
    0xFF, 0x55, 0xFF,   // LightMagenta
    0xFF, 0xFF, 0x55,   // Yellow
    0xFF, 0xFF, 0xFF,   // White
};



// Find closest color
int FindClosestVgaColor(uint32_t color, bool backgroundColor)
{
    const int r = (color >> 16) & 0xFF;
    const int g = (color >> 8) & 0xFF;
    const int b = color & 0xFF;

    int result = 0;
    int bestDistance2 = INT_MAX;

    for (int i = 0; i != (backgroundColor ? 8 : 16); ++i)
    {
        // Refs: https://www.compuphase.com/cmetric.htm
        const int rmean = (vgaColorPalette[i][0] + r) / 2;
        const int dr = (vgaColorPalette[i][0] - r);
        const int dg = (vgaColorPalette[i][1] - g);
        const int db = (vgaColorPalette[i][2] - b);
        const int distance2 = (((512+rmean)*dr*dr)>>8) + 4*dg*dg + (((767-rmean)*db*db)>>8);

        if (distance2 < bestDistance2)
        {
            bestDistance2 = distance2;
            result = i;
        }
    }

    return result;
}
