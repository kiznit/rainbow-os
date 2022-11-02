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

#include "arch.hpp"
#include "SerialPort.hpp"
#include "memory.hpp"
#include <cassert>
#include <metal/log.hpp>

static constexpr mtl::PhysicalAddress kSystemMemoryOffset = 0xFFFF800000000000ull;

void ArchInitEarlyConsole()
{
    mtl::g_log.AddLogger(std::make_shared<SerialPort>());
}

void ArchInitialize()
{
    // The PAT is indexed by page flags (PAT, CacheDisable, WriteThrough)
    const uint64_t patValue = (mtl::PatWriteBack << 0) |         // index 0
                              (mtl::PatWriteThrough << 8) |      // index 1
                              (mtl::PatUncacheableMinus << 16) | // index 2
                              (mtl::PatUncacheable << 24) |      // index 3
                              (mtl::PatWriteCombining << 32);    // index 4

    mtl::WriteMsr(mtl::Msr::IA32_PAT, patValue);
}

void ArchReleaseBootMemory()
{
    const auto cr3 = mtl::Read_CR3();

    // All boot memory should be under pml4[0], so we only need to remove that mapping
    const auto pml4 = reinterpret_cast<uint64_t*>(cr3);
    pml4[0] = 0;

    // Flush all TLBs
    mtl::Write_CR3(cr3);
}

std::expected<void*, ErrorCode> ArchMapSystemMemory(PhysicalAddress physicalAddress, int pageCount, mtl::PageFlags pageFlags)
{
    if (pageFlags & mtl::PageFlags::User) [[unlikely]]
        return std::unexpected(ErrorCode::InvalidArguments);

    if (physicalAddress + pageCount * mtl::kMemoryPageSize > 0x0000800000000000ull) [[unlikely]]
        return std::unexpected(ErrorCode::InvalidArguments);

    const auto virtualAddress = reinterpret_cast<void*>(physicalAddress + kSystemMemoryOffset);
    const auto result = MapPages(physicalAddress, virtualAddress, pageCount, pageFlags);
    if (!result) [[unlikely]]
        return std::unexpected(result.error());

    return virtualAddress;
}

void* ArchGetSystemMemory(PhysicalAddress address)
{
    // TODO: should we verify that this memory was properly mapped before?

    if (address <= 0x0000800000000000ull)
        return reinterpret_cast<void*>(address + kSystemMemoryOffset);

    return nullptr;
}
