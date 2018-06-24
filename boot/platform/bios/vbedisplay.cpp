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

#include "vbedisplay.hpp"
#include "vbe.hpp"
#include <stdio.h>


// TODO: we need to track what low memory is used where within the bootloader
static VbeInfoBlock* g_vbeInfoBlock = (VbeInfoBlock*)0x7000;    // 1024 bytes (play safe, some implementation write more than 512 bytes)
static ModeInfoBlock* g_modeInfoBlock = (ModeInfoBlock*)0x7400; // 256 bytes



bool VbeDisplay::Initialize()
{
    m_vbeVersion = 0;
    m_modeCount = 0;

    VbeInfoBlock& info = *g_vbeInfoBlock;
    if (!vbe_GetInfo(&info))
    {
        return false;
    }

    m_vbeVersion = info.VbeVersion;

    auto oemString = (const char*)(info.OemStringPtr[1] * 16 + info.OemStringPtr[0]);

    printf("VBE version     : %xh\n", info.VbeVersion);
    printf("VBE OEM string  : %s\n", oemString);
    printf("VBE totalMemory : %d MB\n", info.TotalMemory * 16);

    auto videoModes = (const uint16_t*)(info.VideoModePtr[1] * 16 + info.VideoModePtr[0]);

    for (const uint16_t* p = videoModes; *p != 0xFFFF && m_modeCount != MAX_MODE_COUNT; ++p)
    {
        ModeInfoBlock& mode = *g_modeInfoBlock;
        if (!vbe_GetMode(*p, &mode))
        {
            continue;
        }

        // Check for graphics (0x10) + linear frame buffer (0x80)
        if ((mode.ModeAttributes & 0x90) != 0x90)
        {
            continue;
        }

        // Check for direct color mode
        if (mode.MemoryModel != 6)
        {
            continue;
        }

        // Keep this mode
        m_modes[m_modeCount++] = *p;
    }

    printf("VBE usable modes: %d\n", m_modeCount);


    return true;
}



int VbeDisplay::GetModeCount() const
{
    return m_modeCount;
}



bool VbeDisplay::GetMode(int index, DisplayMode* info) const
{
    if (index < 0 || index >= m_modeCount)
    {
        return false;
    }

    ModeInfoBlock& mode = *g_modeInfoBlock;
    if (!vbe_GetMode(m_modes[index], &mode))
    {
        return false;
    }

    if (m_vbeVersion < 0x300)
    {
        const auto redMask = ((1 << mode.RedMaskSize) - 1) << mode.RedFieldPosition;
        const auto greenMask = ((1 << mode.GreenMaskSize) -1 ) << mode.GreenFieldPosition;
        const auto blueMask = ((1 << mode.BlueMaskSize) - 1) << mode.BlueFieldPosition;
        const auto reservedMask = ((1 << mode.RsvdMaskSize) - 1) << mode.RsvdFieldPosition;

        info->width = mode.XResolution;
        info->height = mode.YResolution;
        info->pitch = mode.BytesPerScanLine;
        info->format = DeterminePixelFormat(redMask, greenMask, blueMask, reservedMask);
    }
    else
    {
        const auto redMask = ((1 << mode.LinRedMaskSize) - 1) << mode.LinRedFieldPosition;
        const auto greenMask = ((1 << mode.LinGreenMaskSize) -1 ) << mode.LinGreenFieldPosition;
        const auto blueMask = ((1 << mode.LinBlueMaskSize) - 1) << mode.LinBlueFieldPosition;
        const auto reservedMask = ((1 << mode.LinRsvdMaskSize) - 1) << mode.LinRsvdFieldPosition;

        info->width = mode.XResolution;
        info->height = mode.YResolution;
        info->pitch = mode.LinBytesPerScanLine;
        info->format = DeterminePixelFormat(redMask, greenMask, blueMask, reservedMask);
    }

    return true;
}



bool VbeDisplay::SetMode(int mode) const
{
    (void)mode;
    return false;
}
