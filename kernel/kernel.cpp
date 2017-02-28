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

/*
#include <stdio.h>
#include <kernel/kernel.hpp>
#include <kernel/pmm.hpp>
#include <rainbow/boot.hpp>

#if defined(__i386__) || defined(__x86_64__)
#include <vgaconsole.hpp>

static VgaConsole g_vgaConsole;
VgaConsole* g_console = NULL;
#endif


static void CallGlobalConstructors()
{
    extern void (*__CTOR_LIST__[])();

    uintptr_t count = (uintptr_t) __CTOR_LIST__[0];

    if (count == (uintptr_t)-1)
    {
        count = 0;
        while (__CTOR_LIST__[count + 1])
            ++count;
    }

    for (uintptr_t i = count; i >= 1; --i)
    {
        __CTOR_LIST__[i]();
    }
}




static void InitConsole(const BootInfo* bootInfo)
{
#if defined(__i386__) || defined(__x86_64__)
    if (bootInfo->frameBufferCount == 0)
        return;

    const FrameBufferInfo* fbi = (FrameBufferInfo*)bootInfo->framebuffers;

    if (fbi->type == FrameBufferType_VGAText)
    {
        g_vgaConsole.Initialize((void*)fbi->address, fbi->width, fbi->height);
        g_console = &g_vgaConsole;

        g_vgaConsole.Rainbow();
    }
#else
    (void)bootInfo;
    printf("Rainbow");
#endif

    printf(" - This is the kernel!\n\n");
}



char data[100];

char data2[] = { 1,2,3,4,5,6,7,8,9,10 };


// Kernel entry point
#if defined(__i386__) || defined(__x86_64__)
extern "C" void kernel_main(const BootInfo* bootInfo)
{
    int local;

    CallGlobalConstructors();

    InitConsole(bootInfo);

    printf("BootInfo at : %p\n", bootInfo);
    printf("bss data at : %p\n", data);
    printf("data2 at    : %p\n", data2);
    printf("stack around: %p\n", &local);

    printf("\nInitializing...\n");

    cpu_init();
    printf("    CPU initialized\n");

    pmm_init(bootInfo->memoryDescriptorCount, (MemoryDescriptor*)bootInfo->memoryDescriptors);
    printf("    PMM initialized\n");

    cpu_halt();
}

#elif defined(__arm__)

extern "C" void kernel_main(unsigned r0, unsigned id, const void* atag)
{
    int local;

    CallGlobalConstructors();

    InitConsole(NULL);

    printf("r0          : 0x%08x\n", r0);
    printf("id          : 0x%08x\n", id);
    printf("atag at     : %p\n", atag);
    printf("bss data at : %p\n", data);
    printf("data2 at    : %p\n", data2);
    printf("stack around: %p\n", &local);

    printf("\nInitializing...\n");

    cpu_init();
    printf("    CPU initialized\n");

    //pmm_init(bootInfo->memoryDescriptorCount, (MemoryDescriptor*)bootInfo->memoryDescriptors);
    //printf("    PMM initialized\n");

    cpu_halt();
}

#endif
*/

extern "C" const char* kernel_main()
{
    return "Hello from the kernel!";
}
