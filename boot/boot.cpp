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
#include "boot.hpp"
#include "elfloader.hpp"


// Globals
BootInfo g_bootInfo;
MemoryMap g_memoryMap;


extern "C" int jumpToKernel(void* kernelEntryPoint, BootInfo* bootInfo, uintptr_t stack);


static void* LoadKernel(void* elfLocation, size_t elfSize)
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
        const physaddr_t address = g_memoryMap.AllocateBytes(MemoryType_Kernel, kernelSize, 0xFFFFFFFF, alignment);
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

    return (void*)entry;
}



void Boot(void* kernel, size_t kernelSize)
{
    vmm_init();

    // Load kernel
    const auto kernelEntryPoint = LoadKernel(kernel, kernelSize);

    // Setup kernel stack
    const auto stackSize = align_up(128 * 1024, MEMORY_PAGE_SIZE);
    const auto stack = g_memoryMap.AllocatePages(MemoryType_Kernel, stackSize  >> MEMORY_PAGE_SHIFT);
    vmm_map(stack, KERNEL_STACK_INIT - stackSize, stackSize);

    // Prepare boot info
    g_memoryMap.Sanitize();
    //g_memoryMap.Print();
    g_bootInfo.descriptorCount = g_memoryMap.size();
    g_bootInfo.descriptors = const_cast<MemoryEntry*>(g_memoryMap.begin());

    // Last bits before jumping to kernel
    vmm_enable();

    Log("\nJumping to kernel at %p...\n", kernelEntryPoint);

    const int exitCode = jumpToKernel(kernelEntryPoint, &g_bootInfo, KERNEL_STACK_INIT);

    Fatal("Kernel exited with code %d\n", exitCode);

    for(;;);
}
