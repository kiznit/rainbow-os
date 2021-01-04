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

#ifndef _RAINBOW_BOOT_BIOS_HPP
#define _RAINBOW_BOOT_BIOS_HPP

#include <cstdint>


struct BiosRegisters
{
    uint16_t ds;
    uint16_t es;
    uint16_t fs;
    uint16_t gs;

    union
    {
        uint32_t eflags;
        struct
        {
            uint16_t flags;
        };
    };

    // Order is important! We use pushad / popad for the registers below.

    union
    {
        uint32_t edi;
        struct
        {
            uint16_t di;
        };
    };

    union
    {
        uint32_t esi;
        struct
        {
            uint16_t si;
        };
    };

    union
    {
        uint32_t ebp;
        struct
        {
            uint16_t bp;
        };
    };

    union
    {
        uint32_t esp;
        struct
        {
            uint16_t sp;
        };
    };

    union
    {
        uint32_t ebx;
        struct
        {
            uint16_t bx;
        };
        struct
        {
            uint8_t bl;
            uint8_t bh;
        };
    };

    union
    {
        uint32_t edx;
        struct
        {
            uint16_t dx;
        };
        struct
        {
            uint8_t dl;
            uint8_t dh;
        };
    };

    union
    {
        uint32_t ecx;
        struct
        {
            uint16_t cx;
        };
        struct
        {
            uint8_t cl;
            uint8_t ch;
        };
    };

    union
    {
        uint32_t eax;
        struct
        {
            uint16_t ax;
        };
        struct
        {
            uint8_t al;
            uint8_t ah;
        };
    };
};


// Initialize the BIOS trampoline so that we can call the BIOS
void InstallBiosTrampoline();


// Return value is 'eax'
extern "C" int CallBios(uint8_t interruptNumber, const BiosRegisters* in, BiosRegisters* out);


#endif
