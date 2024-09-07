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

#pragma once

#include "arch.hpp"
#include <rainbow/acpi.hpp>

// Map an ACPI table into system memory so that we can access it
template <std::derived_from<AcpiTable> T = AcpiTable>
const T* AcpiMapTable(mtl::PhysicalAddress address)
{
    return reinterpret_cast<const T*>(ArchGetSystemMemory(address));
}

// Check if the ACPI table contains the specified field.
// Basically this checks that the table's length is big enough for the field to exist.
template <std::derived_from<AcpiTable> T, typename F>
bool AcpiTableContainsImpl(const T* table, const F* field)
{
    const auto minLength = (uintptr_t)field - (uintptr_t)table + sizeof(*field);
    return table->length >= minLength;
}

#define AcpiTableContains(table, field) AcpiTableContainsImpl((table), &(table)->field)
