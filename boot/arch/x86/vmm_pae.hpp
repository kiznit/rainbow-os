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


class VmmPae : public IVirtualMemoryManager
{
public:
    virtual void init()
    {
        // To keep things simple, we are going to identity-map memory up to 0xE0000000.
        // The framebuffer will be mapped at 0xE0000000.
        // The kernel will be mapped at 0xF0000000.

        pml3 = (uint64_t*)g_memoryMap.AllocatePages(MemoryType_Kernel, 1);
        uint64_t* pml2 = (uint64_t*)g_memoryMap.AllocatePages(MemoryType_Kernel, 4);
        uint64_t* pml1 = (uint64_t*)g_memoryMap.AllocatePages(MemoryType_Kernel, 28);

        memset(pml3, 0, MEMORY_PAGE_SIZE);
        memset(pml2, 0, MEMORY_PAGE_SIZE * 4);
        memset(pml1, 0, MEMORY_PAGE_SIZE * 28);

        // 4 entries = 4 x 1GB = 4 GB
        // NOTE: make sure not to put PAGE_WRITE on these 4 entries, it is not legal.
        //       Bochs will validate this and crash. QEMU ignores it.
        pml3[0] = (uintptr_t)&pml2[0] | PAGE_PRESENT;
        pml3[1] = (uintptr_t)&pml2[512] | PAGE_PRESENT;
        pml3[2] = (uintptr_t)&pml2[1024] | PAGE_PRESENT;
        pml3[3] = (uintptr_t)&pml2[1536] | PAGE_PRESENT;

        // 1792 entries = 1792 * 2 MB = 3584 MB
        for (uint64_t i = 0; i != 1792; ++i)
        {
            pml2[i] = i * 512 * MEMORY_PAGE_SIZE | PAGE_LARGE | PAGE_WRITE | PAGE_PRESENT;
        }

        // Initialize PML2 level pages in the kernel area. This is done so that we the kernel
        // can easily clone the kernel address space. Basically these entries will be copied
        // into each task's page table. These are preallocated so that kernel allocations
        // don't have to (this would result in different tasks having a different view of the
        // kernel address space).
        // 28 entries = 28 * 4 KB = 112 KB
        for (uint64_t i = 2016; i != 2044; ++i)
        {
            pml2[i] = (uintptr_t)&pml1[(i - 2016) * 512] | PAGE_WRITE | PAGE_PRESENT | PAGE_GLOBAL;
        }

        // Setup recursive mapping
        //      0xFF7FF000 - 0xFF7FFFFF     Page Mapping Level 3 (PDPT)
        //      0xFF800000 - 0xFFFFBFFF     Page Mapping Level 1 (Page Tables)
        //      0xFFFFC000 - 0xFFFFFFFF     Page Mapping Level 2 (Page Directories)

        // For PAE, we do this at PML2 instead of PML3. This is to save virtual memory space.
        // Recursive mapping at PML3 would consume 1 GB of virtual memory.
        // Doing it at PML2 only uses 8 MB of virtual memory.
        // The is accomplished by mapping the 4 page directories (PML2) in the last 4 entries of
        // the last page directory.

        pml2[2044] = (uintptr_t)&pml2[0] | PAGE_WRITE | PAGE_PRESENT;
        pml2[2045] = (uintptr_t)&pml2[512] | PAGE_WRITE | PAGE_PRESENT;
        pml2[2046] = (uintptr_t)&pml2[1024] | PAGE_WRITE | PAGE_PRESENT;
        pml2[2047] = (uintptr_t)&pml2[1536] | PAGE_WRITE | PAGE_PRESENT;

        // Map the PDPT itself
        map_page((uintptr_t)pml3, 0xFF7FF000, PAGE_WRITE | PAGE_PRESENT);

        // Enable NX
        uint64_t efer = x86_read_msr(MSR_EFER);
        efer |= EFER_NX;
        x86_write_msr(MSR_EFER, efer);

        // Determine supported flags
        supportedFlags = PAGE_NX | 0xFFF;
    }


    virtual void* getPageTable()
    {
        return pml3;
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

        const long i3 = (virtualAddress >> 30) & 0x1FF;
        const long i2 = (virtualAddress >> 21) & 0x1FF;
        const long i1 = (virtualAddress >> 12) & 0x1FF;

        const uint64_t kernelSpaceFlags = (i2 >= 1920 && i2 < 2044) ? PAGE_GLOBAL : 0;

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
            pml2[i2] = page | PAGE_WRITE | PAGE_PRESENT | kernelSpaceFlags;
        }

        uint64_t* pml1 = (uint64_t*)(pml2[i2] & ~(MEMORY_PAGE_SIZE - 1));
        if (pml1[i1] & PAGE_PRESENT)
        {
            Fatal("vmm_map_page() - there is already something there! (i1 = %d, entry = %X)\n", i1, pml1[i1]);
        }

        pml1[i1] = physicalAddress | flags | kernelSpaceFlags;
    }


private:
    physaddr_t supportedFlags;
    uint64_t* pml3;
};
