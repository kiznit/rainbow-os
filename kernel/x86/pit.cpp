/*
    Copyright (c) 2018, Thierry Tremblay
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
#include <metal/x86/io.hpp>

//TODO: remove this dependency
#include "pic.hpp"


static PIT g_pit;
Timer* g_timer = &g_pit;


#define PIT_CHANNEL0 0x40
#define PIT_CHANNEL1 0x41
#define PIT_CHANNEL2 0x42
#define PIT_COMMAND 0x43

#define PIT_INIT_TIMER 0x36     // Channel 0, mode 3, square-wave

#define PIT_FREQUENCY 1193182   // Really, it is 1193181.6666... Hz



void PIT::Initialize(int frequency, InterruptHandler callback)
{
    interrupt_register(PIC_IRQ_OFFSET, callback);

    uint32_t divisor = (frequency > 0) ? PIT_FREQUENCY / frequency : 0;

    // Valid range for divisor is 16 bits (0 is interpreted as 65536)
    if (divisor > 0xFFFF) divisor = 0; // Cap at 18.2 Hz
    else if (divisor < 1) divisor = 1; // Cap at 1193182 Hz

    io_out_8(PIT_COMMAND, PIT_INIT_TIMER);
    io_out_8(PIT_CHANNEL0, divisor & 0xFF);
    io_out_8(PIT_CHANNEL0, divisor >> 8);

// TODO: ugly!
    g_interruptController->Enable(0);
}
