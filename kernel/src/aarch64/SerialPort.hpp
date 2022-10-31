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

#pragma once

#include <metal/arch.hpp>
#include <metal/log.hpp>

/*
    ARM / PrimeCell PL011 UART
*/

class SerialPort : public mtl::Logger
{
public:
    SerialPort(mtl::PhysicalAddress baseAddress, int baseClock);

    void Log(const mtl::LogRecord& record) override;

    void Print(std::u8string_view string);

private:
    // https://developer.arm.com/documentation/ddi0183/g/programmers-model/summary-of-registers
    struct Registers
    {
        uint32_t DR;
        uint32_t RSR_ECR;
        uint8_t reserved1[0x10];
        const uint32_t FR;
        uint8_t reserved2[0x4];
        uint32_t LPR;
        uint32_t IBRD;
        uint32_t FBRD;
        uint32_t LCR_H;
        uint32_t CR;
        uint32_t IFLS;
        uint32_t IMSC;
        const uint32_t RIS;
        const uint32_t MIS;
        uint32_t ICR;
        uint32_t DMACR;
    };

    static_assert(sizeof(Registers) == 0x4C);

    volatile Registers* m_registers;
};
