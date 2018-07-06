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

#include <stdio.h>
#include "bios.hpp"
#include "memory.hpp"
#include "console.hpp"

extern IConsole* g_console;
extern MemoryMap g_memoryMap;



extern "C" int _libc_print(const char* string)
{
    return g_console->Print(string);
}



extern "C" int getchar()
{
    BiosRegisters regs;

    regs.eax = 0;
    CallBios(0x16, &regs, &regs);

    return regs.eax & 0xFF;
}



extern "C" void abort()
{
    getchar();

    //todo: reset system by calling bios

    for (;;)
    {
        asm("cli; hlt");
    }
}



// dlmalloc

#define USE_LOCKS 0
#define NO_MALLOC_STATS 1

#define HAVE_MORECORE 0
#define MMAP_CLEARS 0

#define LACKS_FCNTL_H 1
#define LACKS_SCHED_H 1
#define LACKS_STRINGS_H 1
#define LACKS_SYS_PARAM_H 1
#define LACKS_TIME_H 1
#define LACKS_UNISTD_H 1

#include <dlmalloc.inc>


extern "C"
{
    int errno;
}


extern "C" void* mmap(void* address, size_t length, int prot, int flags, int fd, off_t offset)
{
    (void)address;
    (void)prot;
    (void)flags;
    (void)offset;

    if (length == 0 || fd != -1)
    {
        errno = EINVAL;
        return MAP_FAILED;
    }


    const int pageCount = align_up(length, MEMORY_PAGE_SIZE) >> MEMORY_PAGE_SHIFT;

    const physaddr_t memory = g_memoryMap.AllocatePages(MemoryType_Bootloader, pageCount);

    if (memory == MEMORY_ALLOC_FAILED)
    {
        errno = ENOMEM;
        return MAP_FAILED;
    }

    return (void*)memory;
}



extern "C" int munmap(void* address, size_t length)
{
    (void)address;
    (void)length;

    // We intentionally do not free any memory so that it is still accessible in the next boot stage

    return 0;
}
