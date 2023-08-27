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

#include "Apic.hpp"
#include <metal/log.hpp>

Apic::Apic(void* address) : m_registers(reinterpret_cast<Registers*>(address))
{
}

std::expected<void, ErrorCode> Apic::Initialize()
{
    static_assert((kSpuriousInterrupt & 7) == 7); // Lowest 3 bits need to be set for P6 and Pentium (do we care?)
    static_assert(kSpuriousInterrupt >= 0 && kSpuriousInterrupt <= 255);

    MTL_LOG(Info) << "[APIC] Local APIC initialized at " << m_registers;
    MTL_LOG(Info) << "    ID            : " << GetId();
    MTL_LOG(Info) << "    Version       : " << GetVersion();
    MTL_LOG(Info) << "    Interrupts    : " << GetInterruptCount();

    // TODO: we need to install a spurious interrupt handler
    m_registers->spuriousInterruptVector = (1 << 8) | kSpuriousInterrupt;

    return {};
}
