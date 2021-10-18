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

#include <cstdlib>
#include <metal/arch.hpp>
#include <metal/helpers.hpp>
#include "uefi.hpp"

extern efi::BootServices* g_efiBootServices;


// Configuration
#define HAVE_MORECORE 0
#define LACKS_ERRNO_H 1
#define LACKS_FCNTL_H 1
#define LACKS_SYS_MMAN_H 1
#define LACKS_SYS_TYPES_H 1
#define LACKS_TIME_H 1
#define LACKS_UNISTD_H 1
#define MMAP_CLEARS 0

#define NO_MALLOC_STATS 1
#define USE_LOCKS 0
#define malloc_getpagesize metal::MEMORY_PAGE_SIZE

#define MALLOC_FAILURE_ACTION

// Fake errno.h implementation
#define EINVAL 1
#define ENOMEM 2

// Fake mman.h implementation
#define MAP_PRIVATE 2
#define MAP_ANONYMOUS 4
#define MAP_FAILED ((void*)-1)
#define PROT_READ 1
#define PROT_WRITE 2

static void* mmap(void* address, size_t length, int prot, int flags, int fd, int offset)
{
    (void)address;
    (void)prot;
    (void)flags;
    (void)offset;

    if (length == 0 || fd != -1)
    {
        return MAP_FAILED;
    }

    const auto pageCount = metal::align_up(length, metal::MEMORY_PAGE_SIZE) >> metal::MEMORY_PAGE_SHIFT;

    if (g_efiBootServices)
    {
        efi::PhysicalAddress memory{0};
        auto status = g_efiBootServices->AllocatePages(efi::AllocateAnyPages, efi::EfiLoaderData, pageCount, &memory);
        if (efi::Error(status))
        {
            abort();
        }

        return reinterpret_cast<void*>(memory);
    }
    else
    {
        //TODO:
        //return (void*)g_memoryMap.AllocatePages(MemoryType::Bootloader, pageCount);
        abort();
        return nullptr;
    }
}


static int munmap(void* memory, size_t length)
{
    const auto pageCount = metal::align_up(length, metal::MEMORY_PAGE_SIZE) >> metal::MEMORY_PAGE_SHIFT;

    if (g_efiBootServices)
    {
        g_efiBootServices->FreePages(reinterpret_cast<efi::PhysicalAddress>(memory), pageCount);
    }
    else
    {
        // TODO: we don't have an implementation to free memory from MemoryMap
        // Maybe we don't care... It is set to "MemoryType::Bootloader" and will be
        // freed at the end of kernel initialization
    }

    return 0;
}


// Some compilers will define _MSC_VER/_WIN32/WIN32 when targetting UEFI.
// We do not want this with dlmalloc.
#undef _MSC_VER
#undef _WIN32
#undef WIN32

// Arithmetic on a null pointer treated as a cast from integer to pointer is a GNU extension
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wnull-pointer-arithmetic"
#endif

#include <dlmalloc/dlmalloc.inc>
