/*
    Copyright (c) 2024, Thierry Tremblay
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

#include "ErrorCode.hpp"
#include "interfaces/IClock.hpp"
#include "interfaces/IInterruptHandler.hpp"
#include <cstdint>
#include <metal/atomic.hpp>
#include <metal/expected.hpp>

// Intel 8253 Programmable Interval Timer (PIT)

// TODO: it might be better to use the RTC as a clock and keep the PIT for timers
class Pit : public IClock, public IInterruptHandler
{
public:
    // Valid range for frequency is [18, 1193182]
    mtl::expected<void, ErrorCode> Initialize(int frequency = 1000);

    // IClock
    uint64_t GetTimeNs() const override;

    // IInterruptHandler
    bool HandleInterrupt(InterruptContext* context) override;

private:
    mtl::atomic<uint64_t> m_counter{0}; // Count time in PIT ticks (3579545/3 hz)
    mtl::atomic<uint32_t> m_divisor;    // Programmed divisor
};
