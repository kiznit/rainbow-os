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

#include "Pit.hpp"
#include <metal/arch.hpp>
#include <metal/log.hpp>

using mtl::x86_inb;
using mtl::x86_outb;

constexpr auto PIT_CHANNEL0 = 0x40;
// constexpr auto PIT_CHANNEL1 = 0x41;
// constexpr auto PIT_CHANNEL2 = 0x42;
constexpr auto PIT_COMMAND = 0x43;

// constexpr auto PIT_INIT_COUNTDOWN = 0x30; // Channel 0, mode 0, interrupt on terminal count
constexpr auto PIT_INIT_TIMER = 0x34; // Channel 0, lobyte/hibyte access mode, rate generator mode
// constexpr auto PIT_READ_STATUS = 0xE2; // Read counter 0 status

// PIT frequency is 3579545/3, which is ~1193181.6666666666666666666666...
constexpr auto PIT_FREQUENCY_NUMERATOR = 3579545;
constexpr auto PIT_FREQUENCY_DENOMINATOR = 3;

// Calculate the duration of one PIT tick in nanoseconds.
// We need 32 bits to hold the numerator and 22 bits for the denominator.
// We can shift the numerator left to increase precision.
// The maximum shift is 54, resulting in a 86 bits numerator.
// Dividing 86 bits by 2 bits gives us a final 64 bits multiplier.
// The multiplier is ((1000000000 * PIT_FREQUENCY_DENOMINATOR) << 54) / PIT_FREQUENCY_NUMERATOR.
constexpr uint64_t kMultiplierShift = 54;
constexpr uint64_t kMultiplier = 15097783525125665971ull;

std::expected<void, ErrorCode> Pit::Initialize(int frequency)
{
    if (frequency < 18 || frequency > 11931812)
        return std::unexpected(ErrorCode::InvalidArguments);

    uint32_t divisor = PIT_FREQUENCY_NUMERATOR / (PIT_FREQUENCY_DENOMINATOR * frequency);

    // Valid range for divisor is 16 bits (0 is interpreted as 65536)
    if (divisor > 0xFFFF)
        divisor = 0; // Cap at 18.2 Hz
    else if (divisor < 1)
        divisor = 1; // Cap at 1193182 Hz

    x86_outb(PIT_COMMAND, PIT_INIT_TIMER);
    x86_outb(PIT_CHANNEL0, divisor & 0xFF);
    x86_outb(PIT_CHANNEL0, divisor >> 8);

    divisor = divisor ? divisor : 0x10000;
    m_divisor = divisor;

    const auto frequencyDenom = PIT_FREQUENCY_DENOMINATOR * divisor;
    frequency = (PIT_FREQUENCY_NUMERATOR + frequencyDenom / 2) / frequencyDenom;

    MTL_LOG(Info) << "[PIT] Setting divisor to " << divisor << " (~" << frequency << " Hz)";

    return {};
}

uint64_t Pit::GetTimeNs() const
{
    uint64_t high, low;
    asm volatile("mul %3" : "=a"(low), "=d"(high) : "a"(m_counter), "r"(kMultiplier));
    const uint64_t timeNs = (high << (64 - kMultiplierShift)) | (low >> kMultiplierShift);
    return timeNs;
}

bool Pit::HandleInterrupt(InterruptContext*)
{
    m_counter += m_divisor;
    return true;
}
