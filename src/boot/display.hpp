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

#ifndef _RAINBOW_BOOT_DISPLAY_HPP
#define _RAINBOW_BOOT_DISPLAY_HPP

#include <graphics/surface.hpp>
#include <rainbow/boot.hpp>

class Edid;


struct GraphicsMode
{
    int         width;          // Width in pixels
    int         height;         // Height in pixels
    PixelFormat format;         // Pixel format
};


class IDisplay
{
public:
    // Return how many different modes are supported by the display
    virtual int GetModeCount() const = 0;

    // Return the current framebuffer
    virtual void GetFramebuffer(Framebuffer* fb) const = 0;

    // Get a display mode description
    virtual bool GetMode(int index, GraphicsMode* mode) const  = 0;

    // Change the display mode
    virtual bool SetMode(int index) = 0;

    // Get the display's EDID information
    virtual bool GetEdid(Edid* edid) const  = 0;
};


void SetBestMode(IDisplay* display);


#endif
