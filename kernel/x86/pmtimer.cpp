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

#include "pmtimer.hpp"
#include <mutex>
#include <kernel/acpi.hpp>


bool PMTimer::Detect()
{
    auto fadt = (const Acpi::Fadt*)acpi_find_table(acpi_signature("FACP"));

    if (!fadt || fadt->length <= offsetof(Acpi::Fadt, flags) || fadt->PM_TMR_LEN != 4)
    {
        return false;
    }

    return true;
}


PMTimer::PMTimer()
:   m_clock(0)
{
    auto fadt = (const Acpi::Fadt*)acpi_find_table(acpi_signature("FACP"));

    // Can we use the X_PM_TMR_BLK field?
    if (fadt->length > offsetof(Acpi::Fadt, X_PM_TMR_BLK) && fadt->X_PM_TMR_BLK.address)
    {
        m_address = fadt->X_PM_TMR_BLK;
    }
    else
    {
        m_address.addressSpaceId = Acpi::GenericAddress::Space::SystemIO;
        m_address.registerBitWidth = 32;
        m_address.registerBitShift = 0;
        m_address.address = fadt->PM_TMR_BLK;
    }

    const auto extended = any(fadt->flags & Acpi::Fadt::Flags::TmrValExt);
    m_timerMask = extended ? 0xFFFFFFFF : 0x00FFFFFF;

    // Initialize last known value
    m_lastTimer = acpi_read(m_address) & m_timerMask;
}


uint64_t PMTimer::GetTimeNs() const
{
    const_cast<PMTimer*>(this)->UpdateClock();

// TODO: add unit tests for these calculations

    // We want to calculate timeNs = m_clock * 1e9 / FREQUENCY, but we need to work around overflows (m_clock * 1e9)
    const uint64_t integer = m_clock / FREQUENCY;
    const uint64_t remainder = m_clock % FREQUENCY;

    const uint64_t integerNs = integer * 1000000000;                    // Can overflow, this is fine
    const uint64_t remainderNs = (remainder * 1000000000) / FREQUENCY;  // Will not overfloaw

    const uint64_t timeNs = integerNs + remainderNs;                    // Can overflow, this is fine

    return timeNs;
}


void PMTimer::UpdateClock()
{
    std::lock_guard lock(m_lock);

    const uint32_t currentTimer = acpi_read(m_address) & m_timerMask;
    const uint32_t ticks = (currentTimer - m_lastTimer) & m_timerMask;

    m_lastTimer = currentTimer;
    m_clock += ticks;
}
