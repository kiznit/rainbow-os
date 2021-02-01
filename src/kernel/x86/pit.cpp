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

#include "pit.hpp"
#include <cassert>
#include <metal/x86/io.hpp>

//TODO: remove this dependency
#include "pic.hpp"

#define PIT_CHANNEL0 0x40
#define PIT_CHANNEL1 0x41
#define PIT_CHANNEL2 0x42
#define PIT_COMMAND  0x43

// TODO: use channel
#define PIT_INIT_COUNTDOWN  0x30    // Channel 0, mode 0, interrupt on terminal count
#define PIT_INIT_TIMER      0x34    // Channel 0, mode 2, rate generator
#define PIT_READ_STATUS     0xE2    // Read counter 0 status


// TODO: ugly statics until we can have context in InterruptHandler
static uint64_t         s_clock = 0;
static InterruptHandler s_handler = nullptr;


static int PitInterruptHandler(InterruptContext* context)
{
    ++s_clock;

    if (s_handler)
    {
        return s_handler(context);
    }

    return 1;
}



void PIT::InitCountdown(int milliseconds)
{
    const uint32_t count = (FREQUENCY * milliseconds) / 1000;
    assert(count <= 0xFFFF);

    io_out_8(PIT_COMMAND, PIT_INIT_COUNTDOWN);
    io_out_8(PIT_CHANNEL0, count & 0xFF);
    io_out_8(PIT_CHANNEL0, count >> 8);
}


bool PIT::IsCountdownExpired() const
{
    io_out_8(PIT_COMMAND, PIT_READ_STATUS);
    auto status = io_in_8(PIT_CHANNEL0);
    return status & 0x80;
}


void PIT::Initialize(int frequency, InterruptHandler callback)
{
    s_clock = 0;
    s_handler = callback;
    interrupt_register(PIC_IRQ_OFFSET, PitInterruptHandler);

    uint32_t divisor = (frequency > 0) ? FREQUENCY / frequency : 0;

    // Valid range for divisor is 16 bits (0 is interpreted as 65536)
    if (divisor > 0xFFFF) divisor = 0; // Cap at 18.2 Hz
    else if (divisor < 1) divisor = 1; // Cap at 1193182 Hz

    io_out_8(PIT_COMMAND, PIT_INIT_TIMER);
    io_out_8(PIT_CHANNEL0, divisor & 0xFF);
    io_out_8(PIT_CHANNEL0, divisor >> 8);

    m_divisor = divisor ? divisor : 0x10000;

// TODO: ugly!
    g_interruptController->Enable(0);
}


uint64_t PIT::GetTimeNs() const
{
// TODO: add unit tests for these calculations

    // We want to calculate timeNs = m_clock * m_divisor * 1e9 / FREQUENCY, but we need to work around overflows (m_clock * 1e9)
    const uint64_t integer = s_clock / FREQUENCY;
    const uint64_t remainder = s_clock % FREQUENCY;

    // We use 1000000280 instead of 1e9 to mitigate the impact of not using the exact PIT frequency
    uint64_t integerNs = integer * 1000000280 * m_divisor;          // Can overflow, this is fine

    uint64_t remainderNs = (remainder * 1000000280);                // Will not overfloaw
    integerNs = integerNs + (remainderNs / FREQUENCY) * m_divisor;  // Can overflow, this is fine

    remainderNs = (remainderNs % FREQUENCY) * m_divisor;

    const uint64_t timeNs = integerNs + (remainderNs / FREQUENCY);  // Can overflow, this is fine

    return timeNs;
}
