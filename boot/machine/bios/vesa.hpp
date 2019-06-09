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

#ifndef _RAINBOW_BOOT_VESA_HPP
#define _RAINBOW_BOOT_VESA_HPP


#include <stdint.h>


struct VbeInfo
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
    uint8_t     OemData[256];

} __attribute__((packed));


struct VbeMode
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


#define VBE_LINEAR_FRAMEBUFFER 0x4000


bool vbe_GetCurrentMode(uint16_t* mode);
bool vbe_GetInfo(VbeInfo* info);
bool vbe_GetMode(int mode, VbeMode* info);
bool vbe_GetEdid(uint8_t edid[128]);
bool vbe_SetMode(int mode);


#endif
