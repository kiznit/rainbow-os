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

#include <expected>
#include <metal/arch.hpp>
#include <rainbow/errno.hpp>
#include <rainbow/uefi.hpp>

using PhysicalAddress = mtl::PhysicalAddress;

// Initialize the memory module
void MemoryInitialize(const efi::MemoryDescriptor* descriptors, size_t descriptorCount);

// Allocate contiguous physical memory
std::expected<PhysicalAddress, ErrorCode> AllocFrames(size_t count);

// Free physical memory
void FreeFrames(PhysicalAddress frames, size_t count);

// Commit memory at the specified address, zero-initialize it
// TODO: return an error code
bool VirtualAlloc(void* address, size_t size);

// Free virtual memory
// TODO: return an error code
bool VirtualFree(void* address, size_t size);