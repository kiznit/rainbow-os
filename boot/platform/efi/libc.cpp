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

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <Uefi.h>

#include "boot.hpp"
#include "console.hpp"
#include "memory.hpp"


extern IConsole* g_console;

extern EFI_HANDLE              g_efiImage;
extern EFI_SYSTEM_TABLE*       g_efiSystemTable;
extern EFI_BOOT_SERVICES*      g_efiBootServices;
extern EFI_RUNTIME_SERVICES*   g_efiRuntimeServices;



extern "C" int _libc_print(const char* string)
{
    return g_console->Print(string);
}



extern "C" int getchar()
{
    if (!g_efiSystemTable || !g_efiBootServices)
        return EOF;

    EFI_SIMPLE_TEXT_INPUT_PROTOCOL* input = g_efiSystemTable->ConIn;
    if (!input)
        return EOF;

    for (;;)
    {
        EFI_STATUS status;

        size_t index;
        status = g_efiBootServices->WaitForEvent(1, &input->WaitForKey, &index);
        if (EFI_ERROR(status))
        {
            return EOF;
        }

        EFI_INPUT_KEY key;
        status = input->ReadKeyStroke(input, &key);
        if (EFI_ERROR(status))
        {
            if (status == EFI_NOT_READY)
                continue;

            return EOF;
        }

        return key.UnicodeChar;
    }
}



extern "C" void abort()
{
    getchar();

    if (g_efiRuntimeServices)
    {
        const char* error = "abort()";
        g_efiRuntimeServices->ResetSystem(EfiResetWarm, EFI_ABORTED, strlen(error), (void*)error);
    }

    for (;;)
    {
        asm("cli; hlt");
    }
}



// dlmalloc

// We must undefine some macros defined by MinGW
#undef WIN32
#undef _WIN32
#undef errno

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

    assert(g_efiBootServices && "Out of memory");

    EFI_PHYSICAL_ADDRESS memory = MAX_ALLOC_ADDRESS;

    const auto status = g_efiBootServices->AllocatePages(AllocateMaxAddress, EfiLoaderData, pageCount, &memory);
    if (EFI_ERROR(status))
    {
        errno = ENOMEM;
        return MAP_FAILED;
    }

    return (void*)memory;
}



extern "C" int munmap(void* address, size_t length)
{
    if (!g_efiBootServices)
    {
        errno = EINVAL;
        return -1;
    }

    const int pageCount = align_up(length, MEMORY_PAGE_SIZE) >> MEMORY_PAGE_SHIFT;

    const auto status = g_efiBootServices->FreePages((EFI_PHYSICAL_ADDRESS)address, pageCount);
    if (EFI_ERROR(status))
    {
        errno = EINVAL;
        return -1;
    }

    return 0;
}



// extern "C" void* calloc(size_t count, size_t size)
// {
//     void* p = malloc(count * size);
//     if (p) memset(p, 0, count * size);
//     return p;
// }
