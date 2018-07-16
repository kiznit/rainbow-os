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

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <rainbow/arch.hpp>

#include "arm.hpp"
#include "boot.hpp"
#include "mailbox.hpp"
#include "memory.hpp"


char* PERIPHERAL_BASE;

void libc_initialize();



void* AllocatePages(size_t pageCount, uintptr_t maxAddress)
{
    const physaddr_t memory = g_memoryMap.AllocatePages(MemoryType_Bootloader, pageCount, maxAddress);
    if (memory == MEMORY_ALLOC_FAILED)
    {
        return nullptr;
    }

    return (void*)memory;
}



bool FreePages(void* memory, size_t pageCount)
{
    // TODO: do we want to implement this?
    (void)memory;
    (void)pageCount;

    return true;
}



/*
    Check this out for detecting Raspberry Pi Model:

        https://github.com/mrvn/RaspberryPi-baremetal/tree/master/004-a-t-a-and-g-walk-into-a-baremetal

    Peripheral base address detection:

        https://www.raspberrypi.org/forums/viewtopic.php?t=127662&p=854371
*/

#if defined(__arm__)
extern "C" void raspi_main(unsigned bootDeviceId, unsigned machineId, const void* parameters)
#elif defined(__aarch64__)
extern "C" void raspi_main(const void* parameters)
#endif
{
    // Clear BSS
    extern char _bss_start[];
    extern char _bss_end[];
    memset(_bss_start, 0, _bss_end - _bss_start);

    // Add bootloader (ourself) to memory map
    extern const char bootloader_image_start[];
    extern const char bootloader_image_end[];
    const physaddr_t start = (physaddr_t)&bootloader_image_start;
    const physaddr_t end = (physaddr_t)&bootloader_image_end;
    g_memoryMap.AddBytes(MemoryType_Bootloader, MemoryFlag_ReadOnly, start, end - start);

    // Peripheral base address
    PERIPHERAL_BASE = (char*)(uintptr_t)(arm_cpuid_model() == ARM_CPU_MODEL_ARM1176 ? 0x20000000 : 0x3F000000);
    g_memoryMap.AddBytes(MemoryType_Reserved, 0, (uintptr_t)PERIPHERAL_BASE, 0x01000000);

    libc_initialize();

    // Clear screen and set cursor to (0,0)
    printf("\033[m\033[2J\033[;H");

    // Rainbow
    printf("\033[31mR\033[1ma\033[33mi\033[1;32mn\033[36mb\033[34mo\033[35mw\033[m");

    printf(" Raspberry Pi Bootloader\n\n");
#if defined(__arm__)
    printf("bootDeviceId    : 0x%08x\n", bootDeviceId);
    printf("machineId       : 0x%08x\n", machineId);
#endif
    printf("parameters      : %p\n", parameters);
    printf("cpu_id          : 0x%08x\n", arm_cpuid_id());
    printf("peripheral_base : %p\n", PERIPHERAL_BASE);

    Mailbox mailbox;
    Mailbox::MemoryRange memory;

    if (mailbox.GetARMMemory(&memory) < 0)
        printf("*** Failed to read ARM memory\n");
    else
    {
        printf("ARM memory      : 0x%08x - 0x%08x\n", (unsigned)memory.address, (unsigned)(memory.address + memory.size));
        g_memoryMap.AddBytes(MemoryType_Available, 0, memory.address, memory.size);
    }

    if (mailbox.GetVCMemory(&memory) < 0)
        printf("*** Failed to read VC memory\n");
    else
    {
        printf("VC memory       : 0x%08x - 0x%08x\n", (unsigned)memory.address, (unsigned)(memory.address + memory.size));
        g_memoryMap.AddBytes(MemoryType_Reserved, 0, memory.address, memory.size);
    }

    printf("\n");

    // Ensure that the first memory page is not available by attempting to allocate it.
    // This is required because AllocatePages() returns NULL to indicate errors / out-of-memory condition.
    AllocatePages(1, MEMORY_PAGE_SIZE);

    ProcessBootParameters(parameters, &g_bootInfo, &g_memoryMap);

    if (g_bootInfo.initrdAddress && g_bootInfo.initrdSize)
    {
        g_memoryMap.AddBytes(MemoryType_Bootloader, MemoryFlag_ReadOnly, g_bootInfo.initrdAddress, g_bootInfo.initrdSize);
    }

    Boot();
}
