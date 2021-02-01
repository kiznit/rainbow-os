/*
    Copyright (c) 2021, Thierry Tremblay
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

#include <kernel/pagetable.hpp>
#include <cstring>
#include <kernel/pmm.hpp>
#include <metal/helpers.hpp>


// TODO: can't hardcode these addresses...
static void* s_mmapBegin = (void*)0x40000000;   // Start of memory-map region
static void* s_mmapEnd   = (void*)0x50000000;   // End of memory-map region


// TODO: this is very similar to vmm_allocate_pages(), we need to unify them if possible
void* PageTable::AllocatePages(int pageCount)
{
    // TODO: provide an API to allocate 'x' continuous frames
    for (auto i = 0; i != pageCount; ++i)
    {
        auto frame = pmm_allocate_frames(1);
        s_mmapBegin = advance_pointer(s_mmapBegin, MEMORY_PAGE_SIZE);

        if (s_mmapBegin > s_mmapEnd)
        {
            throw std::bad_alloc();
        }

        // TODO: verify return value
        MapPages(frame, s_mmapBegin, 1, PAGE_PRESENT | PAGE_USER | PAGE_WRITE | PAGE_NX);

        // TODO: we should keep a pool of zero-ed memory
        memset(s_mmapBegin, 0, MEMORY_PAGE_SIZE);
    }

    return s_mmapBegin;
}
