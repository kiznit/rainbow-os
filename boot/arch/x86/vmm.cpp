/*
    Copyright (c) 2018, Thierry Tremblay
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

#include "vmm.hpp"
#include <metal/x86/cpu.hpp>
#include <metal/x86/cpuid.hpp>
#include <rainbow/elf.h>
#include "boot.hpp"

extern MemoryMap g_memoryMap;

#include "vmm_x86.hpp"
#include "vmm_ia32.hpp"
#include "vmm_pae.hpp"
#include "vmm_x86_64.hpp"



static IVirtualMemoryManager* s_vmm;


void vmm_init(int machine)
{
    unsigned int eax, ebx, ecx, edx;

    if (machine == EM_X86_64)
    {
        s_vmm = new VmmLongMode();
    }
    else if (x86_cpuid(1, &eax, &ebx, &ecx, &edx) && (edx & bit_PAE))
    {
        s_vmm = new VmmPae();
    }
    else
    {
        s_vmm = new VmmIa32();
    }

    s_vmm->init();
}


void vmm_enable()
{
    s_vmm->enable();
}


void vmm_map(uint64_t physicalAddress, uint64_t virtualAddress, size_t size, physaddr_t flags)
{
    s_vmm->map(physicalAddress, virtualAddress, size, flags);
}


void vmm_map_page(uint64_t physicalAddress, uint64_t virtualAddress, physaddr_t flags)
{
    s_vmm->map_page(physicalAddress, virtualAddress, flags);
}