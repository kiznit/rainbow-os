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
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.ok
*/

#include "mailbox.hpp"
#include <arch/io.hpp>
#include <stdio.h>
#include <string.h>



#define MBOX_BASE 0xB880     // Base address of the mailbox registers

// Registers

#define MBOX_READ   0x00
#define MBOX_PEEK   0x10
#define MBOX_SENDER 0x14
#define MBOX_STATUS 0x18
#define MBOX_CONFIG 0x1c
#define MBOX_WRITE  0x20


// Status
#define MBOX_FULL   0x80000000
#define MBOX_EMPTY  0x40000000



template<int size>
struct MailboxBuffer
{
    volatile uint32_t m_data[(size+3)/4];
};



template<typename T>
class __attribute__((aligned(16))) MailboxMessage : public MailboxBuffer<sizeof(T) + 6*4>
{
public:

    MailboxMessage(Mailbox::PropertyTag tag)
    {
        this->m_data[0] = sizeof(*this);  // Total size of request, including end tag and padding
        this->m_data[1] = 0;              // Request code
        this->m_data[2] = tag;            // Property tag id
        this->m_data[3] = sizeof(T);      // Size of buffer
        this->m_data[4] = 0;              // Request size
        memset((void*)&this->m_data[5], 0, sizeof(*this) - 5 * 4);
    }

    uint32_t ResponseSize() const   { return this->m_data[4]; }
    const T& Value() const          { return *(const T*)&this->m_data[5]; }
};



// Some sanity checks
static_assert(sizeof(MailboxMessage<Mailbox::MemoryRange>) == 32);




Mailbox::Mailbox(const MachineDescription& machine)
{
    m_registers = machine.peripheral_base + MBOX_BASE;
}



uint32_t Mailbox::Read(uint8_t channel)
{
    for (;;)
    {
        while (mmio_read(m_registers + MBOX_STATUS) & MBOX_EMPTY)
        {
            // Wait for data
        }

        uint32_t data = mmio_read(m_registers + MBOX_READ);
        uint8_t readChannel = data & 0x0F;

        if (readChannel == channel)
        {
            return (data & ~0x0F);
        }
    }
}



int Mailbox::Write(uint8_t channel, uint32_t data)
{
    // Data needs to be 16 bytes aligned!
    if (data & 0x0F)
    {
        printf("Mailbox::Write: data not aligned!");
        return -1;
    }

    while (mmio_read(m_registers + MBOX_STATUS) & MBOX_FULL)
    {
        // Wait for space
    }

    mmio_write(m_registers + MBOX_WRITE, (data & ~0x0F) | (channel & 0xF));

    return 0;
}




int Mailbox::GetMemory(PropertyTag tag, MemoryRange* memory)
{
    MailboxMessage<MemoryRange> request(tag);

    if (Write(Channel_PropertyTags, (uint32_t)&request) < 0)
        return -1;

    uint32_t result = Read(Channel_PropertyTags);
    printf("got result from Mailbox.Read(): 0x%08x\n", (unsigned)result);

    printf("response size: 0x%08x\n", (unsigned)request.ResponseSize());

    // buffer[4] is 0x80000008 : 8 bytes + MSB to indicate a response

    // if (result != 0x80000004)   // 8 bytes + MSB to indicate a response
    //     return -1;

    *memory = request.Value();

    return 0;
}
