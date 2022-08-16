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
#include <cassert>
#include <rainbow/boot.hpp>

extern "C" [[noreturn]] void KernelTrampoline(const BootInfo* bootInfo, const void* kernelEntryPoint, void* ttbr1_el1);

[[noreturn]] void JumpToKernel(const BootInfo& bootInfo, const void* kernelEntryPoint, PageTable& pageTable)
{
    // There are a number of assumptions here:
    //  1) MMU is enabled by UEFI
    //  2) UEFI is only using TTBR0_EL1
    // This means that we are running in low address space and there is no need to relocate a trampoline. We can just jump to the
    // kernel which is in high address space.
    assert(mtl::Read_TTBR0_EL1() != 0); // TODO: I suppose 0 here could be valid, but that doesn't seem likely?
    assert(mtl::Read_TTBR1_EL1() == 0);

    KernelTrampoline(&bootInfo, kernelEntryPoint, pageTable.GetRaw());
}
