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

#pragma once

#include "arch.hpp"
#include "interfaces/IInterruptHandler.hpp"
#include <concepts>
#include <rainbow/acpi.hpp>

class Acpi : private IInterruptHandler
{
public:
    // (ACPI spec section 5.8.1)
    enum class InterruptModel
    {
        PIC = 0,
        APIC = 1,
        SAPIC = 2
    };

    enum class SleepState : uint8_t
    {
        S0 = 0,
        S1 = 1,
        S2 = 2,
        S3 = 3,
        S4 = 4,
        S5 = 5,
        Shutdown = S5,
    };

    [[nodiscard]] std::expected<void, ErrorCode> Initialize(const AcpiRsdp& rsdp);

    // TODO: if we specify the table, we shouldn't need to specify the signature... its implicit
    template <std::derived_from<AcpiTable> T>
    [[nodiscard]] const T* FindTable(std::string_view signature, int index = 0) const
    {
        auto table = FindTable(signature, index);
        return table ? static_cast<const T*>(table) : nullptr;
    }

    [[nodiscard]] const AcpiTable* FindTable(std::string_view signature, int index) const;

    // Enable ACPI
    [[nodiscard]] std::expected<void, ErrorCode> Enable(InterruptModel model);

    // Reset the system
    [[nodiscard]] std::expected<void, ErrorCode> ResetSystem();

    // Put the system to sleep
    [[nodiscard]] std::expected<void, ErrorCode> SleepSystem(SleepState state);

    // Shutdown the system
    [[nodiscard]] std::expected<void, ErrorCode> ShutdownSystem() { return SleepSystem(SleepState::Shutdown); }

    /*
        void EnumerateNamespace();
    */

private:
    bool HandleInterrupt(InterruptContext* context) override;
    bool IsHardwareReduced() const;

    bool m_initialized{};
    bool m_enabled{};
    const AcpiRsdt* m_rsdt{};
    const AcpiXsdt* m_xsdt{};
    const AcpiFadt* m_fadt{};
};
