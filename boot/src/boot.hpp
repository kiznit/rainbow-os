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

#include "MemoryMap.hpp"
#include <expected>
#include <metal/arch.hpp>
#include <rainbow/uefi.hpp>

// Maximum memory address to use for allocations. We to do this to prevent allocations that would overlap with the kernel's address
// range. This simplifies things as the kernel will be able to access all memory allocated by the bootloader without having to map
// it during initialization.
constexpr mtl::PhysicalAddress kMaxAllocationAddress = 1ull << 32;

// Allocate pages of memory (below kMaxAllocationAddress).
// This function will not return on out-of-memory conditions.
// A return value of 0 is valid and doesn't represent an error condition.
mtl::PhysicalAddress AllocatePages(size_t pageCount, efi::MemoryType memoryType);

// Like allocate pages, but clears the memory
mtl::PhysicalAddress AllocateZeroedPages(size_t pageCount, efi::MemoryType memoryType);

// Set a memory range to the specified memory type
void SetCustomMemoryType(mtl::PhysicalAddress address, size_t pageCount, efi::MemoryType memoryType);

// Entry point
efi::Status EfiMain(efi::Handle hImage, efi::SystemTable* systemTable);
