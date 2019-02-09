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


class VmmLongMode : public IVirtualMemoryManager
{
public:
    virtual void init()
    {
        // To keep things simple, we are going to identity-map the first 4 GB of memory.
        // The kernel will be mapped outside of the first 4GB of memory.

        pml4 = (uint64_t*)g_memoryMap.AllocatePages(MemoryType_Kernel, 1);
        pml3 = (uint64_t*)g_memoryMap.AllocatePages(MemoryType_Kernel, 1);
        pml2 = (uint64_t*)g_memoryMap.AllocatePages(MemoryType_Kernel, 4);

        memset(pml4, 0, MEMORY_PAGE_SIZE);
        memset(pml3, 0, MEMORY_PAGE_SIZE);
        memset(pml2, 0, MEMORY_PAGE_SIZE * 4);

        // 1 entry = 512 GB
        pml4[0] = (uintptr_t)pml3 | PAGE_WRITE | PAGE_PRESENT;

        // 4 entries = 4 x 1GB = 4 GB
        pml3[0] = (uintptr_t)&pml2[0] | PAGE_WRITE | PAGE_PRESENT;
        pml3[1] = (uintptr_t)&pml2[512] | PAGE_WRITE | PAGE_PRESENT;
        pml3[2] = (uintptr_t)&pml2[1024] | PAGE_WRITE | PAGE_PRESENT;
        pml3[3] = (uintptr_t)&pml2[1536] | PAGE_WRITE | PAGE_PRESENT;

        // 2048 entries = 2048 * 2 MB = 4 GB
        for (uint64_t i = 0; i != 2048; ++i)
        {
            pml2[i] = i * 512 * MEMORY_PAGE_SIZE | PAGE_LARGE | PAGE_WRITE | PAGE_PRESENT;
        }

        // Setup recursive mapping
        //      0xFFFFFF00 00000000 - 0xFFFFFF7F FFFFFFFF   Page Mapping Level 1 (Page Tables)
        //      0xFFFFFF7F 80000000 - 0xFFFFFF7F BFFFFFFF   Page Mapping Level 2 (Page Directories)
        //      0xFFFFFF7F BFC00000 - 0xFFFFFF7F BFDFFFFF   Page Mapping Level 3 (PDPTs / Page-Directory-Pointer Tables)
        //      0xFFFFFF7F BFDFE000 - 0xFFFFFF7F BFDFEFFF   Page Mapping Level 4 (PML4)

        // We use entry 510 because the kernel occupies entry 511
        pml4[510] = (uintptr_t)pml4 | PAGE_WRITE | PAGE_PRESENT;

        // Determine supported flags
        supportedFlags = 0xFFF;

        unsigned int eax, ebx, ecx, edx;
        if (x86_cpuid(0x80000001, &eax, &ebx, &ecx, &edx))
        {
            if (edx & bit_NX)
            {
                // Enable NX
                uint64_t efer = x86_read_msr(MSR_EFER);
                efer |= EFER_NX;
                x86_write_msr(MSR_EFER, efer);

                supportedFlags |= PAGE_NX;
            }
        }
    }


    virtual void enable()
    {
        x86_set_cr3((uintptr_t)pml4);
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
        flags = (flags & supportedFlags) | PAGE_PRESENT;

        const long i4 = (virtualAddress >> 39) & 0x1FF;
        const long i3 = (virtualAddress >> 30) & 0x1FF;
        const long i2 = (virtualAddress >> 21) & 0x1FF;
        const long i1 = (virtualAddress >> 12) & 0x1FF;

        if (!(pml4[i4] & PAGE_PRESENT))
        {
            const uint64_t page = g_memoryMap.AllocatePages(MemoryType_Kernel, 1);
            pml4[i4] = page | PAGE_WRITE | PAGE_PRESENT;
            memset((void*)page, 0, MEMORY_PAGE_SIZE);
        }

        uint64_t* pml3 = (uint64_t*)(pml4[i4] & ~(MEMORY_PAGE_SIZE - 1));
        if (!(pml3[i3] & PAGE_PRESENT))
        {
            const uint64_t page = g_memoryMap.AllocatePages(MemoryType_Kernel, 1);
            memset((void*)page, 0, MEMORY_PAGE_SIZE);
            pml3[i3] = page | PAGE_WRITE | PAGE_PRESENT;
        }

        uint64_t* pml2 = (uint64_t*)(pml3[i3] & ~(MEMORY_PAGE_SIZE - 1));
        if (!(pml2[i2] & PAGE_PRESENT))
        {
            const uint64_t page = g_memoryMap.AllocatePages(MemoryType_Kernel, 1);
            memset((void*)page, 0, MEMORY_PAGE_SIZE);
            pml2[i2] = page | PAGE_WRITE | PAGE_PRESENT;
        }

        uint64_t* pml1 = (uint64_t*)(pml2[i2] & ~(MEMORY_PAGE_SIZE - 1));
        if (pml1[i1] & PAGE_PRESENT)
        {
            Fatal("vmm_map_page() - there is already something there! (i1 = %d, entry = %X)\n", i1, pml1[i1]);
        }

        pml1[i1] = physicalAddress | flags;
    }


private:
    physaddr_t supportedFlags;
    uint64_t* pml4;
    uint64_t* pml3;
    uint64_t* pml2;
};
