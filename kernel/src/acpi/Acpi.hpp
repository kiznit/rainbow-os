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
#include <concepts>
#include <rainbow/acpi.hpp>

// (ACPI spec section 5.8.1)
enum class AcpiInterruptModel
{
    Pic = 0,
    Apic = 1,
    Sapic = 2
};

enum class AcpiSleepState : uint8_t
{
    S0 = 0,
    S1 = 1,
    S2 = 2,
    S3 = 3,
    S4 = 4,
    S5 = 5,
    Shutdown = S5,
};

mtl::expected<void, ErrorCode> AcpiInitialize(const AcpiRsdp& rsdp);

const AcpiTable* AcpiFindTable(mtl::string_view signature, int index);

// TODO: if we specify the table, we shouldn't need to specify the signature... its implicit
template <std::derived_from<AcpiTable> T>
inline const T* AcpiFindTable(mtl::string_view signature, int index = 0)
{
    auto table = AcpiFindTable(signature, index);
    return table ? static_cast<const T*>(table) : nullptr;
}

// Enable ACPI
mtl::expected<void, ErrorCode> AcpiEnable(AcpiInterruptModel model);

// Reset the system
mtl::expected<void, ErrorCode> AcpiResetSystem();

// Put the system to sleep
mtl::expected<void, ErrorCode> AcpiSleepSystem(AcpiSleepState state);

// Shutdown the system
inline mtl::expected<void, ErrorCode> AcpiShutdownSystem()
{
    return AcpiSleepSystem(AcpiSleepState::Shutdown);
}

// Enumerate the ACPI namespace
void AcpiEnumerateNamespace();
