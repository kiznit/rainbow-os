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
#include "elfloader.hpp"
#include "memory.hpp"


typedef int (*kernel_entry_t)();



static kernel_entry_t LoadKernel(MemoryMap* memoryMap, void* elfLocation, size_t elfSize)
{
    ElfLoader elf(elfLocation, elfSize);

    if (!elf.Valid())
    {
        Fatal("Unsupported: kernel is not a valid elf file\n");
    }

    if (elf.GetType() != ET_EXEC)
    {
        Fatal("Unsupported: kernel is not an executable\n");
    }

#if defined(__i386__)
    if (elf.GetMachine() != EM_386 && elf.GetMachine() != EM_X86_64)
#elif defined(__x86_64__)
    if (elf.GetMachine() != EM_X86_64)
#elif defined(__arm__)
    if (elf.GetMachine() != EM_ARM)
#elif defined(__aarch64__)
    if (elf.GetMachine() != EM_AARCH64)
#endif
    {
        Fatal("Unsupported: kernel architecture (%d)\n", elf.GetMachine());
    }

    const unsigned int kernelSize = elf.GetMemorySize();
    const unsigned int kernelAlignment = elf.GetMemoryAlignment();

#if defined(__i386__)
    const unsigned int largePageSize = MEMORY_LARGE_PAGE_SIZE * 2;  // Don't assume PAE is available
#else
    const unsigned int largePageSize = MEMORY_LARGE_PAGE_SIZE;
#endif

    void* kernel = nullptr;

    for (unsigned int alignment = max(largePageSize, kernelAlignment); alignment >= kernelAlignment; alignment >>= 1)
    {
        const physaddr_t address = memoryMap->AllocateBytes(MemoryType_Kernel, kernelSize, 0xFFFFFFFF, alignment);
        if (address != (physaddr_t)-1)
        {
            kernel = (void*)address;
            break;
        }
    }

    if (!kernel)
    {
        Fatal("Could not allocate memory to load kernel (size: %x, alignment: %x)\n", kernelSize, kernelAlignment);
    }

    Log("Kernel memory allocated at %p - %p\n", kernel, (char*)kernel + kernelSize);

    const physaddr_t entry = elf.Load(kernel);
    if (entry == 0)
    {
        Fatal("Error loading kernel\n");
    }

    return (kernel_entry_t)entry;
}



void Boot(MemoryMap* memoryMap, void* kernel, size_t kernelSize)
{
    vmm_init();

    auto jump_to_kernel = LoadKernel(memoryMap, kernel, kernelSize);

    memoryMap->Sanitize();
    //memoryMap->Print();

    Log("\nJumping to kernel at %p...\n", jump_to_kernel);

    vmm_enable();

    const int exitCode = jump_to_kernel();

    Fatal("Kernel exited with code %d\n", exitCode);

    for(;;);
}
