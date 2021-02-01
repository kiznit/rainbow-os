/*
    Copyright (c) 2021, Thierry Tremblay
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

#ifndef _RAINBOW_KERNEL_X86_PMTIMER_HPP
#define _RAINBOW_KERNEL_X86_PMTIMER_HPP

#include <rainbow/acpi.hpp>
#include <kernel/clock.hpp>
#include <kernel/spinlock.hpp>


// This is the ACPI Power Management Timer
//
// - Frequency is fixed at 3579545 Hz
// - Counter is 24 or 32 bits (wraparound every 4.69 or 1200 seconds).
// - Not affected by power management features (aggressive idling, throttling or frequency scaling).
//
// TODO: support timer wraparound properly. Investigate TMR_STS and/or using an interrupt handler.
//       right now, if we don't call the timer every < 4.69 seconds, it will lose time.
//
// TODO: do we care about extended timer? I don't think we do... We can use it as a 24 bits timers and it would just work
// TODO: if we only use 24 bits, can we improve calculations of clock time? See https://lwn.net/Articles/40407/.


class PMTimer : public IClock
{
public:

    static const int FREQUENCY = 3579545;

    static bool Detect();

    PMTimer();

    // Return the clock time in nanoseconds
    uint64_t GetTimeNs() const override;


private:

    // Update m_clock
    void UpdateClock();

    Acpi::GenericAddress m_address; // Timer address
    uint32_t    m_timerMask;        // Mask to handle 24 vs 32 bits timers

    Spinlock    m_lock;             // Protect the update of the next two fields (m_lastTimer and m_clock)
    uint32_t    m_lastTimer;        // Last value read from the timer
    uint64_t    m_clock;            // Current time in timer ticks (not in nanoseconds)
};


#endif
