/*
    Copyright (c) 2020, Thierry Tremblay
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

#include "boot.hpp"
#include <cstring>

extern "C"
{
    typedef int (*KernelTrampoline)(physaddr_t kernelEntryPoint, BootInfo* bootInfo, void* pageTable);
}


// UEFI could have loaded the bootloader at any address. If the bootloader happens to use addresses
// we want to use for the kernel, we will crash miserably when we set and enable the new page tables.
// The workaround is to relocate a "jump to kernel" trampoline to an address range outside the one
// used by the kernel.

extern "C" int jumpToKernel(physaddr_t kernelEntryPoint, BootInfo* bootInfo, void* pageTable)
{
    extern const char KernelTrampolineStart[];
    extern const char KernelTrampolineEnd[];

    const auto trampolineSize = KernelTrampolineEnd - KernelTrampolineStart;

    KernelTrampoline trampoline = (KernelTrampoline) g_memoryMap.AllocateBytes(MemoryType::Bootloader, trampolineSize);
    memcpy((void*)trampoline, KernelTrampolineStart, trampolineSize);

    return trampoline(kernelEntryPoint, bootInfo, pageTable);
}
