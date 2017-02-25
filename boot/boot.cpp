/*
    Copyright (c) 2017, Thierry Tremblay
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
#include <stdio.h>
#include "elf.hpp"
#include "memory.hpp"



void Boot(BootInfo* bootInfo, MemoryMap* memoryMap)
{
    printf("\nBoot info:\n");
    printf("    initrd address..........: 0x%016llx\n", bootInfo->initrdAddress);
    printf("    initrd size.............: 0x%016llx\n", bootInfo->initrdSize);
    printf("\n");

    // For now, 'initrd' is really the kernel
    const auto kernelStart = (const char*)bootInfo->initrdAddress;
    const auto kernelSize = bootInfo->initrdSize;

    ElfLoader elf(kernelStart, kernelSize);
    if (!elf.Valid())
    {
        printf("Unsupported: kernel is not a valid elf file\n");
        return;
    }

    if (elf.GetType() != ET_EXEC)
    {
        printf("Unsupported: kernel is not an executable\n");
        return;
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
        printf("Unsupported: kernel architecture (%d)\n", elf.GetMachine());
        return;
    }


    const unsigned int elfSize = elf.GetMemorySize();
    const unsigned int elfAlignment = elf.GetMemoryAlignment();

#if defined(__i386__)
    const unsigned int largePageSize = MEMORY_LARGE_PAGE_SIZE * 2;  // Don't assume PAE is available
#else
    const unsigned int largePageSize = MEMORY_LARGE_PAGE_SIZE;
#endif

    void* memory = nullptr;

    for (unsigned int alignment = max(largePageSize, elfAlignment); alignment >= elfAlignment; alignment >>= 1)
    {
        const physaddr_t address = memoryMap->AllocateBytes(MemoryType_Kernel, elfSize, 0xFFFFFFFF, alignment);
        if (address != (physaddr_t)-1)
        {
            memory = (void*)address;
            break;
        }
    }

    if (!memory)
    {
        printf("Could not allocate memory to load kernel (size: %u, alignment: %u)\n", elfSize, elfAlignment);
        return;
    }

    printf("Kernel memory allocated at %p - %p\n", memory, (char*)memory + elfSize);

    physaddr_t entry = elf.Load(memory);
    if (entry == 0)
    {
        printf("Error loading kernel\n");
        return;
    }

    memoryMap->Sanitize();
    //memoryMap->Print();
    bootInfo->memoryDescriptorCount = memoryMap->size();
    bootInfo->memoryDescriptors = (uintptr_t)memoryMap->begin();


    printf("Jumping to kernel...");
}
