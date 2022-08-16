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

#include "PageTable.hpp"
#include "boot.hpp"
#include <cstring>
#include <metal/helpers.hpp>
#include <rainbow/boot.hpp>

using KernelTrampoline = void(const BootInfo* bootInfo, const void* kernelEntryPoint, void* pageTable);

extern "C" {
extern const char KernelTrampolineStart[];
extern const char KernelTrampolineEnd[];
}

[[noreturn]] void JumpToKernel(const BootInfo& bootInfo, const void* kernelEntryPoint, PageTable& pageTable)
{
    const auto trampolineSize = KernelTrampolineEnd - KernelTrampolineStart;
    const auto pageCount = mtl::AlignUp(trampolineSize, mtl::kMemoryPageSize) >> mtl::kMemoryPageShift;

    auto trampoline = (KernelTrampoline*)(uintptr_t)AllocatePages(pageCount);
    memcpy((void*)trampoline, KernelTrampolineStart, trampolineSize);

    trampoline(&bootInfo, kernelEntryPoint, pageTable.GetRaw());

    for (;;)
        ;
}
