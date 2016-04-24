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

#include "colors.hpp"



// This is very approximate but good enough for the needs of the boot console
// TODO: convert sRGB to XYZ, XYZ to Lab and do find better match?
int FindNearestColor(uint32_t color, const uint32_t* palette, int lengthPalette)
{
    const float r = (float)((color >> 16) & 0xFF) / 255.0f;
    const float g = (float)((color >> 8)  & 0xFF) / 255.0f;
    const float b = (float)((color >> 0)  & 0xFF) / 255.0f;

    int result = 0;
    float bestDistance = 4.0f;

    for (int i = 0; i != lengthPalette; ++i)
    {
        const float pr = (float)((palette[i] >> 16) & 0xFF) / 255.0f;
        const float pg = (float)((palette[i] >> 8)  & 0xFF) / 255.0f;
        const float pb = (float)((palette[i] >> 0)  & 0xFF) / 255.0f;

        // Watch out, these can be negative!
        const float dr = r - pr;
        const float dg = g - pg;
        const float db = b - pb;

        float distance = dr * dr + dg * dg + db * db;

        if (distance < bestDistance)
        {
            result = i;
            bestDistance = distance;
        }
    }

    return result;
}
