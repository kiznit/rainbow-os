/*
    Copyright (c) 2016, Thierry Tremblay
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

#ifndef _RAINBOW_BOOT_RASPI_MAILBOX_HPP
#define _RAINBOW_BOOT_RASPI_MAILBOX_HPP

#include "raspi.hpp"


/*
    Mailbox Property Interface

    https://github.com/raspberrypi/firmware/wiki/Mailboxes
*/

class Mailbox
{
public:

    enum Channel
    {
        Channel_PowerManagement = 0,
        Channel_FrameBuffer = 1,
        Channel_VirtualUART = 2,
        Channel_VCHIQ = 3,
        Channel_LEDs = 4,
        Channel_Buttons = 5,
        Channel_TouchScreen = 6,
        Channel_PropertyTags = 8,
    };


    enum PropertyTag
    {
        Tag_ARMMemory = 0x00010005,     // ARM Memory
        Tag_VCMemory = 0x00010006,      // VideoCore Memory
    };


    Mailbox(const MachineDescription& machine);

    // Read / write
    // Valid channel range: 0..0xF
    uint32_t Read(uint8_t channel);
    int Write(uint8_t channel, uint32_t data);


    // Properties
    struct MemoryRange
    {
        uint32_t address;
        uint32_t size;
    };

    int GetARMMemory(MemoryRange* memory)   { return GetMemory(Tag_ARMMemory, memory); }
    int GetVCMemory(MemoryRange* memory)    { return GetMemory(Tag_VCMemory, memory); }


private:

    int GetMemory(PropertyTag tag, MemoryRange* memory);

    uintptr_t m_registers;
};



#endif

