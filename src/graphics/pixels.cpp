/*
    Copyright (c) 2020, Thierry Tremblay
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

#include "pixels.hpp"



PixelFormat DeterminePixelFormat(unsigned int redMask, unsigned int greenMask, unsigned int blueMask, unsigned int reservedMask)
{
    if (redMask == 0xFF0000 && greenMask == 0xFF00 && blueMask == 0xFF && reservedMask == 0)
    {
        return PixelFormat::R8G8B8;
    }

    if (redMask == 0xFF0000 && greenMask == 0xFF00 && blueMask == 0xFF && reservedMask == 0xFF000000)
    {
        return PixelFormat::X8R8G8B8;
    }

    if (redMask == 0xFF && greenMask == 0xFF00 && blueMask == 0xFF0000 && reservedMask == 0xFF000000)
    {
        return PixelFormat::X8B8G8R8;
    }

    return PixelFormat::Unknown;
}



int GetPixelDepth(PixelFormat format)
{
    switch (format)
    {
        case PixelFormat::R8G8B8:
            return 3;

        case PixelFormat::X8R8G8B8:
        case PixelFormat::X8B8G8R8:
            return 4;

        default:
            return 0;
    }
}
