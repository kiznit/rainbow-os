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

#include "GenericTimer.hpp"
#include "acpi/Acpi.hpp"
#include <metal/math.hpp>

mtl::expected<mtl::unique_ptr<GenericTimer>, ErrorCode> GenericTimer::Create()
{
    const auto gtdt = AcpiFindTable<AcpiGenericTimer>("GTDT");
    if (!gtdt)
    {
        MTL_LOG(Fatal) << "[GTMR] Generic timer not found in ACPI";
        return mtl::unexpected(ErrorCode::Unsupported);
    }

    MTL_LOG(Info) << "[GTMR] EL1 Timer GSIV: " << gtdt->nonSecureEL1TimerGsiv;
    MTL_LOG(Info) << "[GTMR] EL1 Timer Flags: " << mtl::hex(gtdt->nonSecureEL1TimerFlags);

    return mtl::unique_ptr(new GenericTimer());
}

GenericTimer::GenericTimer() : m_frequency(mtl::Read_CNTFRQ_EL0())
{
    MTL_LOG(Info) << "[GTMR] EL1 Timer Frequency: " << m_frequency;

    auto num = 1000000000ull;
    auto den = m_frequency;
    const auto gcd = std::gcd(num, den);
    num /= gcd;
    den /= gcd;

    // If denominator is 1, this is easy
    if (den == 1)
    {
        m_multiplier = num;
        m_shift = 0;
    }
    else
    {
        const auto numBits = mtl::log2(num) + 1;
        const auto denBits = mtl::log2(den) + 1;

        m_shift = 64 + numBits - denBits;
        m_shift = std::min(m_shift, 64 - numBits); // TODO - If we had 128 by 64 division, we could remove this line: more precision
        m_multiplier = (num << m_shift) / den;     // TODO: would need 128 bits division here
    }
}

uint64_t GenericTimer::GetTimeNs() const
{
    mtl::aarch64_isb_sy();
    const auto counter = mtl::Read_CNTPCT_EL0();

    uint64_t high, low;
    asm volatile("mul %0, %2, %3;"
                 "umulh %1, %2, %3;"
                 : "=&r"(low), "=&r"(high)
                 : "r"(counter), "r"(m_multiplier));

    const uint64_t timeNs = (high << (64 - m_shift)) | (low >> m_shift);
    return timeNs;
}

void GenericTimer::Start(uint64_t timeoutNs)
{
    m_signaled = false;

    //  TODO: handle overflows
    const auto count = (timeoutNs * m_frequency) / 1000000000;
    mtl::Write_CNTP_TVAL_EL0(count);
    mtl::Write_CNTP_CTL_EL0(1);
}

bool GenericTimer::IsSignaled() const
{
    return m_signaled;
}

bool GenericTimer::HandleInterrupt(InterruptContext*)
{
    mtl::Write_CNTP_CTL_EL0(0);
    m_signaled = true;
    return true;
}
