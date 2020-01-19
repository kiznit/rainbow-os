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

#include "vesa.hpp"
#include <metal/crt.hpp>
#include "bios.hpp"


// Sanity checks
static_assert(sizeof(VbeInfo) == 512);
static_assert(sizeof(VbeMode) == 256);



bool vbe_GetCurrentMode(uint16_t* mode)
{
    BiosRegisters regs;
    regs.ax = 0x4F03;

    CallBios(0x10, &regs, &regs);

    *mode = regs.dx & 0x3FFF;

    return regs.ax == 0x4F;
}



bool vbe_GetInfo(VbeInfo* info)
{
    memset(info, 0, sizeof(*info));
    info->VbeSignature[0] = 'V';
    info->VbeSignature[1] = 'B';
    info->VbeSignature[2] = 'E';
    info->VbeSignature[3] = '2';

    BiosRegisters regs;
    regs.ax = 0x4F00;
    regs.es = (uintptr_t)info >> 4;
    regs.di = (uintptr_t)info & 0xF;

    CallBios(0x10, &regs, &regs);

    return regs.ax == 0x4F;
}



bool vbe_GetMode(int mode, VbeMode* info)
{
    memset(info, 0, sizeof(*info));

    BiosRegisters regs;
    regs.ax = 0x4F01;
    regs.cx = mode;
    regs.es = (uintptr_t)info >> 4;
    regs.di = (uintptr_t)info & 0xF;

    CallBios(0x10, &regs, &regs);

    return regs.ax == 0x4F;
}



bool vbe_GetEdid(uint8_t edid[128])
{
    memset(edid, 0, sizeof(*edid));

    BiosRegisters regs;
    regs.ax = 0x4F15;
    regs.bx = 1;
    regs.cx = 0;
    regs.dx = 0;
    regs.es = (uintptr_t)edid >> 4;
    regs.di = (uintptr_t)edid & 0xF;

    CallBios(0x10, &regs, &regs);

    return regs.ax == 0x4F;
}



bool vbe_SetMode(int mode)
{
    BiosRegisters regs;
    regs.ax = 0x4F02;
    regs.bx = mode;

    CallBios(0x10, &regs, &regs);

    return regs.ax == 0x4F;
}
