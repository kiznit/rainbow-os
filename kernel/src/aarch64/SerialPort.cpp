/*
    Copyright (c) 2022, Thierry Tremblay
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
#include "arch.hpp"

constexpr const char8_t* kSeverityText[6] = {u8"Trace  ", u8"Debug  ", u8"Info   ", u8"Warning", u8"Error  ", u8"Fatal  "};

constexpr auto CR_TXE = 1 << 8;       // Transmit enable
constexpr auto CR_UARTEN = 1 << 0;    // UART enable
constexpr auto FR_TXFF = 1 << 5;      // Transmit FIFO full
constexpr auto FR_BUSY = 1 << 3;      // UART busy transmitting data
constexpr auto LCR_H_WLEN_8 = 3 << 5; // 8 bits transmission
constexpr auto LCR_H_FEN = 1 << 4;    // FIFO enable

SerialPort::SerialPort(mtl::PhysicalAddress baseAddress, int clock)
    : m_registers((Registers*)ArchMapSystemMemory(baseAddress, 1, mtl::PageFlags::MMIO).value()), m_clock(clock)
{
    Reset();
}

void SerialPort::Log(const mtl::LogRecord& record)
{
    Print(kSeverityText[(int)record.severity]);
    Print(u8": ");
    Print(record.message);
    Print(u8"\n\r");
}

void SerialPort::Print(std::u8string_view string) const
{
    for (char c : string)
    {
        while (m_registers->FR & FR_TXFF)
            ;

        m_registers->DR = c;
    }
}

void SerialPort::Reset()
{
    // Disable the UART
    m_registers->CR = 0;

    // Flush FIFOs
    m_registers->LCR_H = 0;

    // Wait for end of transmission
    while (m_registers->FR & FR_BUSY)
        ;

    // Set baud rate
    const auto value = (m_clock * 4 + m_baud / 2) / m_baud;
    const auto integer = value / 64;
    const auto fraction = value & 63;
    m_registers->IBRD = integer;
    m_registers->FBRD = fraction;

    // Enable FIFOs, 8 bits
    m_registers->LCR_H = LCR_H_FEN | LCR_H_WLEN_8;

    // Mask all interrupts
    m_registers->IMSC = 0x7ff;

    // Disable DMA
    m_registers->DMACR = 0;

    // Enable UART transmission
    m_registers->CR = CR_TXE | CR_UARTEN;
}
