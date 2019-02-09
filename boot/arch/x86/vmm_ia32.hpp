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


class VmmIa32 : public IVirtualMemoryManager
{
public:
    virtual void init()
    {
        // To keep things simple, we are going to identity-map memory up to 0xE0000000.
        // The framebuffer will be mapped at 0xE0000000.
        // The kernel will be mapped at 0xF0000000.

        pml2 = (uint32_t*)g_memoryMap.AllocatePages(MemoryType_Kernel, 1);

        memset(pml2, 0, MEMORY_PAGE_SIZE);

        // 896 entries = 896 * 4 MB = 3584 MB
        for (unsigned i = 0; i != 896; ++i)
        {
            pml2[i] = i * 1024 * MEMORY_PAGE_SIZE | PAGE_LARGE | PAGE_WRITE | PAGE_PRESENT;
        }

        // Setup recursive mapping
        //      0xFFC00000 - 0xFFFFEFFF     Page Mapping Level 1 (Page Tables)
        //      0xFFFFF000 - 0xFFFFFFFF     Page Mapping Level 2 (Page Directory)
        pml2[1023] = (uintptr_t)pml2 | PAGE_WRITE | PAGE_PRESENT;
    }

    virtual void enable()
    {
        // Note: this code assumes paging is not enabled

        // Enable PSE (4 MB pages) - todo: do we care if the CPU is so old it doesn't support PSE?
        uint32_t cr4 = x86_get_cr4();
        cr4 |= 1 << 4; // bit 4 = PSE enable
        x86_set_cr4(cr4);

        // Setup page tables
        x86_set_cr3((uintptr_t)pml2);

        // Enable paging
        uint32_t cr0 = x86_get_cr0();
        cr0 |= 1 << 31; // bit 31 = Paging enable
        x86_set_cr0(cr0);
    }


    virtual void map(uint64_t physicalAddress, uint64_t virtualAddress, size_t size, physaddr_t flags)
    {
        size = align_up(size, MEMORY_PAGE_SIZE);

        while (size > 0)
        {
            vmm_map_page(physicalAddress, virtualAddress, flags);
            size -= MEMORY_PAGE_SIZE;
            physicalAddress += MEMORY_PAGE_SIZE;
            virtualAddress += MEMORY_PAGE_SIZE;
        }
    }


    virtual void map_page(uint64_t physicalAddress, uint64_t virtualAddress, physaddr_t flags)
    {
        flags |= PAGE_PRESENT;

        const long i2 = (virtualAddress >> 22) & 0x3FF;
        const long i1 = (virtualAddress >> 12) & 0x3FF;

        if (!(pml2[i2] & PAGE_PRESENT))
        {
            const uint32_t page = g_memoryMap.AllocatePages(MemoryType_Kernel, 1);
            pml2[i2] = page | PAGE_WRITE | PAGE_PRESENT;
            memset((void*)(uintptr_t)page, 0, MEMORY_PAGE_SIZE);
        }

        uint32_t* pml1 = (uint32_t*)(uintptr_t)(pml2[i2] & ~(MEMORY_PAGE_SIZE - 1));
        if (pml1[i1] & PAGE_PRESENT)
        {
            Fatal("vmm_map_page() - there is already something there! (i1 = %d, entry = %X)\n", i1, pml1[i1]);
        }

        pml1[i1] = physicalAddress | flags;
    }


private:
    uint32_t* pml2;
};
