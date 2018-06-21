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


struct VbeInfoBlock
{
    // VBE 1.0
    char        VbeSignature[4];
    uint16_t    VbeVersion;
    uint16_t    OemStringPtr[2];
    uint8_t     Capabilities[4];
    uint16_t    VideoModePtr[2];
    uint16_t    TotalMemory;       // Number of 64KB blocks

    // VBE 2.0
    uint16_t    OemSoftwareRev;
    uint16_t    OemVendorNamePtr[2];
    uint16_t    OemProductNamePtr[2];
    uint16_t    OemProductRevPtr[2];

    // Reserved
    uint8_t     Reserved[222];

} __attribute__((packed));

static_assert(sizeof(VbeInfoBlock) == 256);


// todo: use offical property names https://web.archive.org/web/20081211174813/http://docs.ruudkoot.nl/vbe20.txt
struct ModeInfoBlock
{
    // VBE 1.0
    uint16_t    ModeAttributes;
    uint8_t     WinAAttributes;
    uint8_t     WinBAttributes;
    uint16_t    WinGranularity;
    uint16_t    WinSize;
    uint16_t    WinASegment;
    uint16_t    WinBSegment;
    uint16_t    WinFuncPtr[2];
    uint16_t    BytesPerScanLine;

    // VBE 1.2
    uint16_t    XResolution;
    uint16_t    YResolution;
    uint8_t     XCharSize;
    uint8_t     YCharSize;
    uint8_t     NumberOfPlanes;
    uint8_t     BitsPerPixel;
    uint8_t     NumberOfBanks;
    uint8_t     MemoryModel;
    uint8_t     BankSize;
    uint8_t     NumberOfImagePages;
    uint8_t     Reserved0;

    // Direct Color Fields (direct/6 and YUV/7 memory models)
    uint8_t     RedMaskSize;
    uint8_t     RedFieldPosition;
    uint8_t     GreenMaskSize;
    uint8_t     GreenFieldPosition;
    uint8_t     BlueMaskSize;
    uint8_t     BlueFieldPosition;
    uint8_t     RsvdMaskSize;
    uint8_t     RsvdFieldPosition;
    uint8_t     DirectColorModeInfo;

    // VBE 2.0
    void*       PhysBasePtr;
    uint32_t    Reserved1;
    uint16_t    Reserved2;

    // VBE 3.0
    uint16_t    LinBytesPerScanLine;
    uint8_t     BnkNumberOfImagePages;
    uint8_t     LinNumberOfImagePages;
    uint8_t     LinRedMaskSize;
    uint8_t     LinRedFieldPosition;
    uint8_t     LinGreenMaskSize;
    uint8_t     LinGreenFieldPosition;
    uint8_t     LinBlueMaskSize;
    uint8_t     LinBlueFieldPosition;
    uint8_t     LinRsvdMaskSize;
    uint8_t     LinRsvdFieldPosition;
    uint32_t    MaxPixelClock;

    // Reserved
    uint8_t     Reserved[190];

} __attribute__((packed));

static_assert(sizeof(ModeInfoBlock) == 256);


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
    vbeInfoBlock->VbeSignature[0] = 'V';
    vbeInfoBlock->VbeSignature[1] = 'B';
    vbeInfoBlock->VbeSignature[2] = 'E';
    vbeInfoBlock->VbeSignature[3] = '2';

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

    auto oemString = (const char*)(info->OemStringPtr[1] * 16 + info->OemStringPtr[0]);

    printf("VBE version     : %xh\n", info->VbeVersion);
    printf("VBE OEM string  : %s\n", oemString);
    printf("VBE totalMemory : %d MB\n", info->TotalMemory * 16);

    // If VBE version is not at least 0x200, we can't retrieve the frame buffer address
    if (info->VbeVersion < 0x200)
    {
        return false;
    }

    auto videoModes = (const uint16_t*)(info->VideoModePtr[1] * 16 + info->VideoModePtr[0]);

    for (const uint16_t* p = videoModes; *p != 0xFFFF && m_modeCount != MAX_MODE_COUNT; ++p)
    {
        const ModeInfoBlock* mode = vbe_GetMode(*p);
        if (!mode)
        {
            continue;
        }

        // Check for graphics (0x10) + linear frame buffer (0x80)
        if ((modeInfoBlock->ModeAttributes & 0x90) != 0x90)
        {
            continue;
        }

        // Check for direct color mode
        if (modeInfoBlock->MemoryModel != 6)
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

    info->width = mode->XResolution;
    info->height = mode->YResolution;
    info->pitch = mode->BytesPerScanLine;

    const auto redMask = ((1 << mode->RedMaskSize) - 1) << mode->RedFieldPosition;
    const auto greenMask = ((1 << mode->GreenMaskSize) -1 ) << mode->GreenFieldPosition;
    const auto blueMask = ((1 << mode->BlueMaskSize) - 1) << mode->BlueFieldPosition;
    const auto reservedMask = ((1 << mode->RsvdMaskSize) - 1) << mode->RsvdFieldPosition;

    info->format = DeterminePixelFormat(redMask, greenMask, blueMask, reservedMask);
    info->refreshRate = 0;       // VBE isn't telling us explicitly

    return true;
}



bool VbeDisplay::SetMode(int mode) const
{
    (void)mode;
    return false;
}
