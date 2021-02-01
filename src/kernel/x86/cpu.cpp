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

#include "cpu.hpp"
#include <cassert>
#include <kernel/vmm.hpp>



std::vector<Cpu*> g_cpus;


void* Cpu::operator new(size_t size)
{
    assert(size <= MEMORY_PAGE_SIZE);
    return vmm_allocate_pages(1);
}


void Cpu::operator delete(void* p)
{
    vmm_free_pages(p, 1);
}



Cpu::Cpu(int id, int apicId, bool enabled, bool bootstrap)
:   id(id),
    apicId(apicId),
    enabled(enabled),
    bootstrap(bootstrap),
    gdt((GdtDescriptor*)vmm_allocate_pages(1)), // TODO: error handling
    tss(&this->tss_),
    task(nullptr)
#if defined(__x86_64__)
    ,userStack(0),
    kernelStack(0)
#endif
{
    InitGdt();
    InitTss();
}
