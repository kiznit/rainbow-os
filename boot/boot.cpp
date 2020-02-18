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

#if defined(__i386__)
#include <metal/x86/cpuid.hpp>
#endif


// Globals
BootInfo g_bootInfo;
MemoryMap g_memoryMap;


#if defined(KERNEL_X86_64)
extern "C" int jumpToKernel64(uint64_t kernelEntryPoint, BootInfo* bootInfo);
#else
extern "C" int jumpToKernel(void* kernelEntryPoint, BootInfo* bootInfo);
#endif


static physaddr_t LoadKernel(void* elfLocation, size_t elfSize)
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


    vmm_init(elf.GetMachine());


    const unsigned int kernelSize = elf.GetMemorySize();
    const unsigned int kernelAlignment = elf.GetMemoryAlignment();

#if defined(KERNEL_X86_64)
    const unsigned int largePageSize = 4*1024*1024; // Long mode has 4MB large pages
#else
    const unsigned int largePageSize = 2*1024*1024; // PAE has 2MB large pages
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

    return entry;
}



// We want to make sure the framebuffer is mapped outside the kernel space.
static void RemapConsoleFramebuffer()
{
#if defined(__i386__) && !defined(KERNEL_X86_64)
    if (g_bootInfo.framebufferCount > 0)
    {
        Framebuffer* fb = &g_bootInfo.framebuffers[0];
        const physaddr_t start = fb->pixels;
        const size_t size = fb->height * fb->pitch;
        const physaddr_t newAddress = 0xE0000000;
        vmm_map(start, newAddress, size);
    }
#endif
}



#if defined(__i386__)
static void CheckCpu()
{
    Log("\nChecking system...\n");

    bool ok;

#if defined(KERNEL_X86_64)
    const bool hasLongMode = cpuid_has_longmode();

    if (!hasLongMode) Log("    Processor does not support long mode (64 bits)\n");

    ok = hasLongMode;
#else
    const bool hasPae = cpuid_has_pae();
    const bool hasNx = cpuid_has_nx();

    if (!hasPae) Log("    Processor does not support Physical Address Extension (PAE)\n");
    if (!hasNx) Log("    Processor does not support no-execute memory protection (NX/XD)\n");

    ok = hasPae && hasNx;
#endif

    if (ok)
    {
        Log("Your system meets the requirements to run Rainbow OS\n");
    }
    else
    {
        Log("Your system does not meet the requirements to run Rainbow OS\n");
        for (;;);
    }
}
#endif




void Boot(void* kernel, size_t kernelSize)
{
#if defined(__i386__)
    CheckCpu();
#endif

    Log("\nBooting...\n\n");

    // Load kernel
    const auto kernelEntryPoint = LoadKernel(kernel, kernelSize);

    // Prepare boot info
    g_memoryMap.Sanitize();
    //g_memoryMap.Print();
    g_bootInfo.descriptorCount = g_memoryMap.size();
    g_bootInfo.descriptors = (uintptr_t)g_memoryMap.begin();

    // Last bits before jumping to kernel
    Log("\nJumping to kernel at %X...\n", kernelEntryPoint);

    RemapConsoleFramebuffer();

    vmm_enable();

#if defined(KERNEL_X86_64)
    const int exitCode = jumpToKernel64(kernelEntryPoint, &g_bootInfo);
#else
    const int exitCode = jumpToKernel((void*)kernelEntryPoint, &g_bootInfo);
#endif

    Fatal("Kernel exited with code %d\n", exitCode);

    for(;;);
}
