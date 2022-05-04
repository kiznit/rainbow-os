/*
    Copyright (c) 2022, Thierry Tremblay
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

#pragma once

#include "Surface.hpp"
#include "VideoMode.hpp"
#include <memory>

namespace mtl
{
    class Edid;

    class IDisplay
    {
    public:
        virtual ~IDisplay() {}

        // Returns how many different modes are supported by the display
        virtual int GetModeCount() const = 0;

        // Get the current mode
        // Note: the mode index cannot be reliably determined on some firmwares, so we don't return
        // it.
        virtual void GetCurrentMode(GraphicsMode* mode) const = 0;

        // Get a display mode description
        virtual bool GetMode(int index, GraphicsMode* mode) const = 0;

        // Change the display mode
        virtual bool SetMode(int index) = 0;

        // Get access to the frontbuffer, if it is accessible.
        // This can return nullptr, the pixel format could be anything / unusuable.
        virtual std::shared_ptr<Surface> GetFrontbuffer() = 0;

        // Get access to the backbuffer
        // The pixel format is always going to be PixelFormat::X8R8G8B8.
        virtual std::shared_ptr<Surface> GetBackbuffer() = 0;

        // Blit pixels from the backbuffer to the framebuffer
        virtual void Blit(int x, int y, int width, int height) = 0;

        // // Get the display's EDID information
        // virtual bool GetEdid(Edid* edid) const = 0;
    };
} // namespace mtl
