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

#include "vbedisplay.hpp"

#include <cstring>
#include <graphics/edid.hpp>
#include <graphics/simpledisplay.hpp>
#include <metal/helpers.hpp>
#include <metal/log.hpp>
#include <rainbow/boot.hpp>

#include "memory.hpp"
#include "vesa.hpp"

extern MemoryMap g_memoryMap;


static PixelFormat DeterminePixelFormat(const VbeMode* mode)
{
    // Check for graphics (0x10) + linear frame buffer (0x80)
    if ((mode->ModeAttributes & 0x90) != 0x90)
    {
        return PixelFormat::Unknown;
    }

    // Check for direct color mode
    if (mode->MemoryModel != 6)
    {
        return PixelFormat::Unknown;
    }

    auto redMask = ((1 << mode->RedMaskSize) - 1) << mode->RedFieldPosition;
    auto greenMask = ((1 << mode->GreenMaskSize) - 1) << mode->GreenFieldPosition;
    auto blueMask = ((1 << mode->BlueMaskSize) - 1) << mode->BlueFieldPosition;
    auto reservedMask = ((1 << mode->RsvdMaskSize) - 1) << mode->RsvdFieldPosition;

    return DeterminePixelFormat(redMask, greenMask, blueMask, reservedMask);
}



void VbeDisplay::Initialize(const Framebuffer& framebuffer)
{
    m_frontbuffer = new Surface {
        framebuffer.width, framebuffer.height, framebuffer.pitch, framebuffer.format, (void*)framebuffer.pixels
    };

    m_info = (VbeInfo*)g_memoryMap.AllocateBytes(MemoryType::Bootloader, sizeof(*m_info), 0x100000);
    m_mode = (VbeMode*)g_memoryMap.AllocateBytes(MemoryType::Bootloader, sizeof(*m_mode), 0x100000);
    m_modeCount = 0;
    m_modes = nullptr;

    if (vbe_GetInfo(m_info))
    {
        m_modeCount = 0;
        m_modes = (const uint16_t*)(m_info->VideoModePtr[1] * 16 + m_info->VideoModePtr[0]);

        for (const uint16_t* p = m_modes; *p != 0xFFFF; ++p)
        {
            ++m_modeCount;
        }
    }

    InitBackbuffer();
}


void VbeDisplay::InitBackbuffer()
{
    const int width = m_frontbuffer->width;
    const int height = m_frontbuffer->height;

    if (m_backbuffer && m_backbuffer->width == width && m_backbuffer->height == height)
    {
        return;
    }

    if (m_backbuffer)
    {
        delete [] (uint32_t*)m_backbuffer->pixels;
    }
    else
    {
        m_backbuffer = new Surface();
    }

    m_backbuffer->width = width;
    m_backbuffer->height = height;
    m_backbuffer->pitch = width * 4;
    m_backbuffer->format = PixelFormat::X8R8G8B8;
    m_backbuffer->pixels = new uint32_t[width * height];
}


int VbeDisplay::GetModeCount() const
{
    return m_modeCount;
}


void VbeDisplay::GetCurrentMode(GraphicsMode* mode) const
{
    mode->width = m_frontbuffer->width;
    mode->height = m_frontbuffer->height;
    mode->format = m_frontbuffer->format;
}


bool VbeDisplay::GetMode(int index, GraphicsMode* mode) const
{
    if (index < 0 || index >= m_modeCount)
    {
        return false;
    }

    if (!vbe_GetMode(m_modes[index], m_mode))
    {
        return false;
    }

    mode->width = m_mode->XResolution;
    mode->height = m_mode->YResolution;
    mode->format = DeterminePixelFormat(m_mode);

    return true;
}


bool VbeDisplay::SetMode(int index)
{
    if (index < 0 || index >= m_modeCount)
    {
        return false;
    }

    if (!vbe_SetMode(m_modes[index] | VBE_LINEAR_FRAMEBUFFER))
    {
        return false;
    }

    if (!vbe_GetMode(m_modes[index], m_mode))
    {
        Fatal("Could not retrieve graphics mode info after changing mode");
    }

    m_frontbuffer->width = m_mode->XResolution;
    m_frontbuffer->height = m_mode->YResolution;
    m_frontbuffer->format = DeterminePixelFormat(m_mode);
    m_frontbuffer->pitch = m_mode->BytesPerScanLine;
    m_frontbuffer->pixels = m_mode->PhysBasePtr;

    InitBackbuffer();

    return true;
}


bool VbeDisplay::GetEdid(Edid* edid) const
{
    uint8_t data[128];
    if (vbe_GetEdid(data))
    {
        return edid->Initialize(data, 128);
    }
    else
    {
        return false;
    }
}
