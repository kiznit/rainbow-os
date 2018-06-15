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

    uint8_t red_mask, red_position;
    uint8_t green_mask, green_position;
    uint8_t blue_mask, blue_position;
    uint8_t rsv_mask, rsv_position;
    uint8_t directcolor_attributes;

    void* framebuffer;
    uint32_t reserved1;
    uint16_t reserved2;
} __attribute__((packed));


struct Edid
{
    char edid[128];
    char vdif[128];
} __attribute__((packed));



// TODO: we need to track what low memory is used where within the bootloader

static VbeInfoBlock* vbeInfoBlock = (VbeInfoBlock*)0x7000;      // 512 bytes
static ModeInfoBlock* modeInfoBlock = (ModeInfoBlock*)0x7200;   // 256 bytes
static Edid* edid = (Edid*)0x7300;   // 256 bytes



bool vbe_Edid()
{
    memset(edid, 0, sizeof(*edid));

    BiosRegisters regs;

    // edid
    regs.eax = 0x4F15;
    regs.ebx = 1;
    regs.ecx = 0;
    regs.edx = 0;
    regs.es = (uintptr_t)edid->edid >> 4;
    regs.edi = (uintptr_t)edid->edid & 0xF;
    CallBios(0x10, &regs);
    if (regs.eax & 0xFF00)
    {
        printf("*** FAILED TO READ EDID: %08lx\n", regs.eax);
        //return false;
    }
    else
    {
        printf("*** GOT EDID\n");
    }

    // vdif
    regs.eax = 0x4F15;
    regs.ebx = 1;
    regs.ecx = 0;
    regs.edx = 0;
    regs.es = (uintptr_t)edid->vdif >> 4;
    regs.edi = (uintptr_t)edid->vdif & 0xF;
    CallBios(0x10, &regs);
    if (regs.eax & 0xFF00)
    {
        printf("*** FAILED TO READ VDIF: %08lx\n", regs.eax);
    }
    else
    {
        printf("*** GOT VDIF\n");
    }

    return true;
}



void vbe_EnumerateDisplayModes()
{
    vbeInfoBlock->signature[0] = 'V';
    vbeInfoBlock->signature[1] = 'B';
    vbeInfoBlock->signature[2] = 'E';
    vbeInfoBlock->signature[3] = '2';

    BiosRegisters regs;
    regs.eax = 0x4F00;
    regs.es = (uintptr_t)vbeInfoBlock >> 4;
    regs.edi = (uintptr_t)vbeInfoBlock & 0xF;

    CallBios(0x10, &regs);

    if (regs.eax != 0x4F)
    {
        return;
    }

    auto oemString = (const char*)(vbeInfoBlock->oemStringPtr[1] * 16 + vbeInfoBlock->oemStringPtr[0]);
    auto videoModes = (const uint16_t*)(vbeInfoBlock->videoModePtr[1] * 16 + vbeInfoBlock->videoModePtr[0]);

    printf("VBE version     : %08x\n", vbeInfoBlock->version);
    printf("VBE OEM string  : %s\n", oemString);
    printf("VBE videoModes  : %p\n", videoModes);
    printf("VBE totalMemory : %08x\n", vbeInfoBlock->totalMemory);

    int mode = 0;
    for (const uint16_t* p = videoModes; *p != 0xFFFF; ++p, ++mode)
    {
        regs.eax = 0x4F01;
        regs.ecx = *p;
        regs.es = (uintptr_t)modeInfoBlock >> 4;
        regs.edi = (uintptr_t)modeInfoBlock & 0xF;

        CallBios(0x10, &regs);

        // Check for error
        if (regs.eax != 0x4F)
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

        //printf("%04x: %d x %d x %d\n", *p, modeInfoBlock->width, modeInfoBlock->height, modeInfoBlock->bpp);
    }
}
