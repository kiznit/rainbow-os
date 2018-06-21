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

#include "vbe.hpp"
#include <stdio.h>
#include <string.h>
#include "bios.hpp"


// todo: use offical property names https://web.archive.org/web/20081211174813/http://docs.ruudkoot.nl/vbe20.txt
struct VbeInfoBlock
{
    char signature[4];
    uint16_t version;
    uint16_t oemStringPtr[2];
    uint8_t capabilities[4];
    uint16_t videoModePtr[2];
    uint16_t totalMemory;       // Number of 64KB blocks
} __attribute__((packed));


// todo: use offical property names https://web.archive.org/web/20081211174813/http://docs.ruudkoot.nl/vbe20.txt
struct ModeInfoBlock
{
    uint16_t attributes;
    uint8_t windowA;
    uint8_t windowB;
    uint16_t windowGranularity;
    uint16_t windowSize;
    uint16_t windowSegmentA;
    uint16_t windowSegmentB;
    uint16_t windowFunctionPtr[2];
    uint16_t pitch; // bytes per scanline

    uint16_t width, height;
    uint8_t charWidth, charHeight, planes, bpp, banks;
    uint8_t memory_model, bank_size, image_pages;
    uint8_t reserved0;

    uint8_t red_mask, red_shift;
    uint8_t green_mask, green_shift;
    uint8_t blue_mask, blue_shift;
    uint8_t reserved_mask, reserved_shift;
    uint8_t directcolor_attributes;

    void* framebuffer;
    uint32_t reserved1;
    uint16_t reserved2;
} __attribute__((packed));


struct Edid
{
    uint8_t data[128];
} __attribute__((packed));



// TODO: we need to track what low memory is used where within the bootloader

static VbeInfoBlock* vbeInfoBlock = (VbeInfoBlock*)0x7000;      // 1024 bytes (play safe, some implementation write more than 512 bytes)
static ModeInfoBlock* modeInfoBlock = (ModeInfoBlock*)0x7400;   // 256 bytes
static Edid* edid = (Edid*)0x7500;                              // 128 bytes



static const VbeInfoBlock* vbe_GetInfo()
{
    memset(vbeInfoBlock, 0, sizeof(VbeInfoBlock));
    vbeInfoBlock->signature[0] = 'V';
    vbeInfoBlock->signature[1] = 'B';
    vbeInfoBlock->signature[2] = 'E';
    vbeInfoBlock->signature[3] = '2';

    BiosRegisters regs;
    regs.ax = 0x4F00;
    regs.es = (uintptr_t)vbeInfoBlock >> 4;
    regs.di = (uintptr_t)vbeInfoBlock & 0xF;

    CallBios(0x10, &regs);

    if (regs.ax != 0x004F)
    {
        printf("*** FAILED TO READ VBEINFOBLOCK: %04x\n", regs.ax);
        return nullptr;
    }

    return vbeInfoBlock;
}



static const ModeInfoBlock* vbe_GetMode(uint16_t mode)
{
    memset(modeInfoBlock, 0, sizeof(ModeInfoBlock));

    BiosRegisters regs;
    regs.ax = 0x4F01;
    regs.cx = mode;
    regs.es = (uintptr_t)modeInfoBlock >> 4;
    regs.di = (uintptr_t)modeInfoBlock & 0xF;

    CallBios(0x10, &regs);

    // Check for error
    if (regs.ax != 0x004F)
    {
        return nullptr;
    }

    return modeInfoBlock;
}



const uint8_t* vbe_Edid()
{
    memset(edid, 0, sizeof(*edid));

    BiosRegisters regs;

    // edid
    regs.ax = 0x4F15;
    regs.bx = 1;
    regs.cx = 0;
    regs.dx = 0;
    regs.es = (uintptr_t)edid->data >> 4;
    regs.di = (uintptr_t)edid->data & 0xF;

    CallBios(0x10, &regs);

    if (regs.ax != 0x004F)
    {
        printf("*** FAILED TO READ EDID: %04x\n", regs.ax);
        return nullptr;
    }
    else
    {
        printf("*** GOT EDID\n");
    }

    return edid->data;
}



bool VbeDisplay::Initialize()
{
    m_modeCount = 0;

    const VbeInfoBlock* info = vbe_GetInfo();
    if (!info)
    {
        return false;
    }

    auto oemString = (const char*)(info->oemStringPtr[1] * 16 + info->oemStringPtr[0]);

    printf("VBE version     : %08x\n", info->version);
    printf("VBE OEM string  : %s\n", oemString);
    printf("VBE totalMemory : %d MB\n", info->totalMemory * 16);

    auto videoModes = (const uint16_t*)(info->videoModePtr[1] * 16 + info->videoModePtr[0]);

    for (const uint16_t* p = videoModes; *p != 0xFFFF && m_modeCount != MAX_MODE_COUNT; ++p)
    {
        const ModeInfoBlock* mode = vbe_GetMode(*p);
        if (!mode)
        {
            continue;
        }

        // Check for graphics (0x10) + linear frame buffer (0x80)
        if ((modeInfoBlock->attributes & 0x90) != 0x90)
        {
            continue;
        }

        // Check for direct color mode
        if (modeInfoBlock->memory_model != 6)
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

    const ModeInfoBlock* mode = vbe_GetMode(m_modes[index]);
    if (!mode)
    {
        return false;
    }

    info->width = mode->width;
    info->height = mode->height;
    info->pitch = mode->pitch;

    const auto redMask = ((1 << mode->red_mask) - 1) << mode->red_shift;
    const auto greenMask = ((1 << mode->green_mask) -1 ) << mode->green_shift;
    const auto blueMask = ((1 << mode->blue_mask) - 1) << mode->blue_shift;
    const auto reservedMask = ((1 << mode->reserved_mask) - 1) << mode->reserved_shift;

    info->format = DeterminePixelFormat(redMask, greenMask, blueMask, reservedMask);
    info->refreshRate = 0;       // VBE isn't telling us explicitly

    return true;
}



bool VbeDisplay::SetMode(int mode) const
{
    (void)mode;
    return false;
}
