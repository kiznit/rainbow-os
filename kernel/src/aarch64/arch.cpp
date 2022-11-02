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

// TODO: Do we want to do this on AARCH64? At the same offset?
static constexpr mtl::PhysicalAddress kSystemMemoryOffset = 0xFFFF800000000000ull;

void ArchInitialize()
{
    const uint64_t mairValue = (mtl::MairWriteBack << 0) |      // index 0
                               (mtl::MairWriteThrough << 8) |   // index 1
                               (mtl::MairUncacheable << 16) |   // index 2
                               (mtl::MairWriteCombining << 24); // index 3

    mtl::Write_MAIR_EL1(mairValue);

    // TODO: address should be taken from device tree, not hardcoded
    mtl::g_log.AddLogger(std::make_shared<SerialPort>(0x9000000, 24000000));
}

void ArchReleaseBootMemory()
{
    // UEFI and the bootloader will map all their allocated memory using TTBR0.
    // So all we need to do here is disable translation through TTBR0.

    const auto tcr = (mtl::Read_TCR_EL1() & ~0xFFFF) | mtl::TCR::EOD0;
    mtl::Write_TCR_EL1(tcr);
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
