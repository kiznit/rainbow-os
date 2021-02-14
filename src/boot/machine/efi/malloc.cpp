/*
    Copyright (c) 2021, Thierry Tremblay
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

#include <cerrno>
#include <rainbow/uefi.h>
#include "boot.hpp"

extern EFI_BOOT_SERVICES* g_efiBootServices;


// GCC is smart enough to optimize malloc() + memset() into calloc(). This results
// in an infinite loop when calling calloc() because it is basically implemented
// by calling malloc() + memset(). This will disable the optimization.
#pragma GCC optimize "no-optimize-strlen"


// Configuration
#define HAVE_MORECORE 0
#define LACKS_SYS_MMAN_H 1
#define LACKS_TIME_H 1
#define MMAP_CLEARS 0

#define NO_MALLOC_STATS 1
#define USE_LOCKS 0
#define malloc_getpagesize MEMORY_PAGE_SIZE

// Fake mman.h implementation
#define MAP_PRIVATE 2
#define MAP_ANONYMOUS 4
#define MAP_FAILED ((void*)-1)
#define PROT_READ 1
#define PROT_WRITE 2


static void* mmap(void* address, size_t length, int prot, int flags, int fd, off_t offset)
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

    if (g_efiBootServices)
    {
        EFI_PHYSICAL_ADDRESS memory = MAX_ALLOC_ADDRESS - 1;
        EFI_STATUS status = g_efiBootServices->AllocatePages(AllocateMaxAddress, EfiLoaderData, pageCount, &memory);
        if (EFI_ERROR(status))
        {
            Fatal("Out of memory");
        }

        return (void*)memory;
    }
    else
    {
        return (void*)g_memoryMap.AllocatePages(MemoryType::Bootloader, pageCount);
    }
}


static int munmap(void* memory, size_t length)
{
    const int pageCount = align_up(length, MEMORY_PAGE_SIZE) >> MEMORY_PAGE_SHIFT;

    if (g_efiBootServices)
    {
        g_efiBootServices->FreePages((EFI_PHYSICAL_ADDRESS)memory, pageCount);
    }
    else
    {
        // TODO: we don't have an implementation to free memory from MemoryMap
        // Maybe we don't care... It is set to "MemoryType::Bootloader" and will be
        // freed at the end of kernel intiialization
    }

    return 0;
}


#include <dlmalloc/dlmalloc.inc>
