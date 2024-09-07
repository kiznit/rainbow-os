/*
    Copyright (c) 2024, Thierry Tremblay
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

#include <cassert>
#include <cstring>
#include <metal/graphics/SimpleDisplay.hpp>
#include <metal/helpers.hpp>

namespace mtl
{
    SimpleDisplay::SimpleDisplay(mtl::shared_ptr<Surface> frontbuffer, mtl::shared_ptr<Surface> backbuffer)
        : m_frontbuffer(frontbuffer), m_backbuffer(backbuffer)
    {
        assert(frontbuffer);
        assert(backbuffer);

        assert(frontbuffer->width == backbuffer->width);
        assert(frontbuffer->height == backbuffer->height);
        assert(frontbuffer->format == backbuffer->format);
        assert(frontbuffer->format == PixelFormat::X8R8G8B8); // TODO: eventually we want to support "any" frontbuffer format
    }

    void SimpleDisplay::InitializeBackbuffer()
    {
        assert(m_backbuffer == m_frontbuffer);

        const auto width = m_frontbuffer->width;
        const auto height = m_frontbuffer->height;

        m_backbuffer = mtl::make_shared<Surface>(width, height, PixelFormat::X8R8G8B8);

        // Copying the frontbuffer to the backbuffer is slow on real hardware, so we don't do it.
        memset(m_backbuffer->pixels, 0, m_backbuffer->height * m_backbuffer->pitch);
    }

    int SimpleDisplay::GetModeCount() const
    {
        return 0;
    }

    void SimpleDisplay::GetCurrentMode(GraphicsMode* mode) const
    {
        mode->width = m_frontbuffer->width;
        mode->height = m_frontbuffer->height;
        mode->format = m_frontbuffer->format;
    }

    bool SimpleDisplay::GetMode(int index, GraphicsMode* mode) const
    {
        (void)index;
        (void)mode;

        return false;
    }

    bool SimpleDisplay::SetMode(int index)
    {
        (void)index;
        return false;
    }

    mtl::shared_ptr<Surface> SimpleDisplay::GetFrontbuffer()
    {
        return m_frontbuffer;
    }

    mtl::shared_ptr<Surface> SimpleDisplay::GetBackbuffer()
    {
        return m_backbuffer;
    }

    void SimpleDisplay::Blit(int x, int y, int width, int height)
    {
        if (m_backbuffer == m_frontbuffer)
        {
            return;
        }

        // TODO: do we want to validate or clamp the parameters? I am not sure I care...

        for (int i = 0; i != height; ++i)
        {
            const auto src = AdvancePointer(m_backbuffer->pixels, (y + i) * m_backbuffer->pitch + x * 4);
            const auto dst = AdvancePointer(m_frontbuffer->pixels, (y + i) * m_frontbuffer->pitch + x * 4);
            memcpy(dst, src, width * 4);
        }
    }

    // bool SimpleDisplay::GetEdid(Edid* edid) const
    // {
    //     (void)edid;
    //     return false;
    // }
} // namespace mtl