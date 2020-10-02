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

#ifndef _RAINBOW_KERNEL_X86_APIC_HPP
#define _RAINBOW_KERNEL_X86_APIC_HPP

#include <stdint.h>


// APIC: Advanced Programmable Interrupt Controller.
// Each CPU has an integrated APIC which is commonly referred to as the "local APIC".
// Very old processors don't have an APIC.


#define APIC_REGISTER_TABLE \
    APIC_REGISTER(0x020, ID) \
    APIC_REGISTER(0x030, VERSION) \
    APIC_REGISTER(0x080, TASK_PRIORITY) \
    APIC_REGISTER(0x090, ARBITRATION_PRIORITY) \
    APIC_REGISTER(0x0A0, APIC_PROCESSOR_PRIORITY) \
    APIC_REGISTER(0x0B0, EOI) \
    APIC_REGISTER(0x0C0, REMOTE_READ) \
    APIC_REGISTER(0x0D0, LOGICAL_DESTINATION) \
    APIC_REGISTER(0x0E0, DESTINATION_FORMAT) \
    APIC_REGISTER(0x0F0, SPURIOUS_VECTOR) \
    APIC_REGISTER(0x100, ISR0) \
    APIC_REGISTER(0x110, ISR1) \
    APIC_REGISTER(0x120, ISR2) \
    APIC_REGISTER(0x130, ISR3) \
    APIC_REGISTER(0x140, ISR4) \
    APIC_REGISTER(0x150, ISR5) \
    APIC_REGISTER(0x160, ISR6) \
    APIC_REGISTER(0x170, ISR7) \
    APIC_REGISTER(0x180, TMR0) \
    APIC_REGISTER(0x190, TMR1) \
    APIC_REGISTER(0x1A0, TMR2) \
    APIC_REGISTER(0x1B0, TMR3) \
    APIC_REGISTER(0x1C0, TMR4) \
    APIC_REGISTER(0x1D0, TMR5) \
    APIC_REGISTER(0x1E0, TMR6) \
    APIC_REGISTER(0x1F0, TMR7) \
    APIC_REGISTER(0x200, IRR0) \
    APIC_REGISTER(0x210, IRR1) \
    APIC_REGISTER(0x220, IRR2) \
    APIC_REGISTER(0x230, IRR3) \
    APIC_REGISTER(0x240, IRR4) \
    APIC_REGISTER(0x250, IRR5) \
    APIC_REGISTER(0x260, IRR6) \
    APIC_REGISTER(0x270, IRR7) \
    APIC_REGISTER(0x280, ERROR_STATUS) \
    APIC_REGISTER(0x2F0, LVT_CMCI) \
    APIC_REGISTER(0x300, ICR0) \
    APIC_REGISTER(0x310, ICR1) \
    APIC_REGISTER(0x320, LVT_TIMER) \
    APIC_REGISTER(0x330, LVT_THERMAL_SENSOR) \
    APIC_REGISTER(0x340, LVT_PERF_COUNTER) \
    APIC_REGISTER(0x350, LVT_LINT0) \
    APIC_REGISTER(0x360, LVT_LINT1) \
    APIC_REGISTER(0x370, LVT_ERROR) \
    APIC_REGISTER(0x380, TIMER_INITIAL_COUNT) \
    APIC_REGISTER(0x390, TIMER_CURRENT_COUNT) \
    APIC_REGISTER(0x3E0, TIMER_DIVISOR)


#define APIC_REGISTER(offset, name) \
    const int APIC_##name = offset;

APIC_REGISTER_TABLE;


void apic_init();



extern void* s_apic;

static inline uint32_t apic_read(int offset)
{
    return *(volatile uint32_t*)((uintptr_t)s_apic + offset);
}

static inline void apic_write(int offset, uint32_t value)
{
    *(volatile uint32_t*)((uintptr_t)s_apic + offset) = value;
}



#endif
