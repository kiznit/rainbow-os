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

#include "ErrorCode.hpp"
#include <expected>
#include <metal/arch.hpp>
#include <rainbow/uefi.hpp>
#include <vector>

using PhysicalAddress = mtl::PhysicalAddress;

// Early memory initialization
void MemoryEarlyInit(std::vector<efi::MemoryDescriptor> memoryMap);

// Initialize the memory module
void MemoryInitialize();

// Find the memory descriptor for the specified address or nullptr if none was found
const efi::MemoryDescriptor* MemoryFindSystemDescriptor(PhysicalAddress address);

// Allocate contiguous physical memory
std::expected<PhysicalAddress, ErrorCode> AllocFrames(int count);

// Free physical memory
std::expected<void, ErrorCode> FreeFrames(PhysicalAddress frames, int count);

// Allocate virtual memory pages
std::expected<void*, ErrorCode> AllocPages(int pageCount);

// Free virtual memory pages
std::expected<void, ErrorCode> FreePages(void* pages, int pageCount);

// Commit memory at the specified address
// Memory will be zero-initialized
std::expected<void, ErrorCode> VirtualAlloc(void* address, int size);

// Free virtual memory
std::expected<void, ErrorCode> VirtualFree(void* address, int size);

// Arch specific
std::expected<void, ErrorCode> MapPages(efi::PhysicalAddress physicalAddress, const void* virtualAddress, int pageCount,
                                        mtl::PageFlags pageFlags);

std::expected<void, ErrorCode> UnmapPages(const void* virtualAddress, int pageCount);

// Helper to figure out page mapping flags
// Returns 0 if the descriptor doesn't have cacheability flags
mtl::PageFlags MemoryGetPageFlags(const efi::MemoryDescriptor& descriptor);
