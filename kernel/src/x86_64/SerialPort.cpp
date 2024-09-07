/*
    Copyright (c) 2023, Thierry Tremblay
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

#include "SerialPort.hpp"
#include <metal/arch.hpp>

constexpr const char8_t* kSeverityText[6] = {u8"Trace  ", u8"Debug  ", u8"Info   ", u8"Warning", u8"Error  ", u8"Fatal  "};

SerialPort::SerialPort(uint16_t port) : m_port(port)
{
}

void SerialPort::Log(const mtl::LogRecord& record)
{
    Print(kSeverityText[(int)record.severity]);
    Print(u8": ");
    Print(record.message);
    Print(u8"\n");
}

void SerialPort::Print(mtl::u8string_view string)
{
    for (char c : string)
    {
        // Wait for transmit buffer to be available
        while (!(mtl::x86_inb(m_port + 5) & 0x20))
            ;

        // Send character
        mtl::x86_outb(m_port, c);
    }
}
