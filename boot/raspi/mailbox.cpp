/*
    Copyright (c) 2017, Thierry Tremblay
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
#include <stdio.h>
#include <string.h>
#include <rainbow/arch.hpp>


extern char* PERIPHERAL_BASE;


#define MBOX_BASE 0xB880     // Base address of the mailbox registers

// Registers

#define MBOX_READ   (MBOX_BASE + 0x00)
#define MBOX_PEEK   (MBOX_BASE + 0x10)
#define MBOX_SENDER (MBOX_BASE + 0x14)
#define MBOX_STATUS (MBOX_BASE + 0x18)
#define MBOX_CONFIG (MBOX_BASE + 0x1c)
#define MBOX_WRITE  (MBOX_BASE + 0x20)


// Status
#define MBOX_FULL   0x80000000
#define MBOX_EMPTY  0x40000000



struct MailboxMessageHeader
{
    enum Code
    {
        Code_Request = 0,
        Code_Success = 0x80000000,
        Code_Error = 0x80000001,
    };

    uint32_t m_size;        // Total size of message
    uint32_t m_code;        // Request or response code
};



template<typename T>
class __attribute__((aligned(16))) MailboxMessage : private MailboxMessageHeader
{
public:

    MailboxMessage(Mailbox::PropertyTag tag)
    {
        m_size = sizeof(*this);                 // Total size of request, including end tag and padding
        m_code = Code_Request;                  // Request code
        m_tag = tag;                            // Property tag id
        m_sizeBuffer = sizeof(m_buffer);        // Size of buffer
        m_sizeValue = 0;                        // Size of value in buffer
        memset(&m_buffer, 0, sizeof(m_buffer)); // Clear the buffer
        m_endTag = Mailbox::Tag_End;            // End tag
    }

    // Accessors
    uint32_t ResponseSize() const       { return (m_sizeValue & 0x80000000) ? m_sizeValue & 0x7FFFFFFF : 0; }
    const T& Value() const              { return m_buffer; }


private:

    uint32_t m_tag;         // Tag
    uint32_t m_sizeBuffer;  // Size of buffer
    uint32_t m_sizeValue;   // Size of value in buffer + request / response indicator in MSB (0 - request, 1 - response)
    T        m_buffer;      // Buffer for request and response values
    uint32_t m_endTag;      // End tag
};



// Some sanity checks
static_assert(sizeof(MailboxMessage<Mailbox::MemoryRange>) == 32);



uint32_t Mailbox::Read(uint8_t channel)
{
    for (;;)
    {
        while (mmio_read32(PERIPHERAL_BASE + MBOX_STATUS) & MBOX_EMPTY)
        {
            // Wait for data
        }

        uint32_t data = mmio_read32(PERIPHERAL_BASE + MBOX_READ);
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

    while (mmio_read32(PERIPHERAL_BASE + MBOX_STATUS) & MBOX_FULL)
    {
        // Wait for space
    }

    mmio_write32(PERIPHERAL_BASE + MBOX_WRITE, (data & ~0x0F) | (channel & 0xF));

    return 0;
}




int Mailbox::GetMemory(PropertyTag tag, MemoryRange* memory)
{
    MailboxMessage<MemoryRange> request(tag);

    if (Write(Channel_PropertyTags, (uint32_t)(uintptr_t)&request) < 0)
        return -1;

    Read(Channel_PropertyTags);

    if (request.ResponseSize() != 8)
        return -1;

    *memory = request.Value();

    return 0;
}



// int Mailbox::GetEDIDBlock(int blockIndex, EDIDBlock* data)
// {
//     MailboxMessage<EDIDBlock> request(Tag_EDID);

//     request.Value().blockIndex = blockIndex;

//     if (Write(Channel_PropertyTags, (uint32_t)&request) < 0)
//         return -1;

//     uint32_t result = Read(Channel_PropertyTags);
//     printf("got result from Mailbox.Read(): 0x%08x\n", (unsigned)result);

//     *data = request.Value();

//     return request.Value().status;
// }
