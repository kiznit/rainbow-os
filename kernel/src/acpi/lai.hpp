/*
    Copyright (c) 2022, Thierry Tremblay
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

#include <lai/core.h>
#include <metal/arch.hpp>
#include <rainbow/acpi.hpp>
#include <type_traits>

static constexpr mtl::PhysicalAddress kAcpiMemoryOffset = 0xFFFF800000000000ull;

class LaiState
{
public:
    LaiState() { lai_init_state(&m_state); }
    ~LaiState() { lai_finalize_state(&m_state); }

    LaiState(const LaiState&) = delete;
    LaiState& operator=(const LaiState&) = delete;

private:
    lai_state_t m_state;
};

// This is a helper to map ACPI tables in memory
// LAI doesn't issue calls to laihost_map() for tables.
template <typename T>
concept AcpiTable = std::is_base_of<acpi::Table, T>::value;

template <AcpiTable T = acpi::Table>
const T* AcpiMapTable(mtl::PhysicalAddress address)
{
    return reinterpret_cast<const T*>(address + kAcpiMemoryOffset);
}
