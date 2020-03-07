/*
    Copyright (c) 2020, Thierry Tremblay
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

#ifndef _RAINBOW_RAINBOW_H
#define _RAINBOW_RAINBOW_H

#include "syscall.h"
#include <stddef.h>
#include <stdint.h>

#if defined(__i386__)
#include <rainbow/arch/ia32/syscall.h>
#elif defined(__x86_64__)
#include <rainbow/arch/x86_64/syscall.h>
#endif


typedef intptr_t off_t;

#ifdef __cplusplus
extern "C" {
#endif


static inline int Log(const char* message)
{
    return syscall1(SYSCALL_LOG, (intptr_t)message);
}


// Linux:
//  void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
//  int munmap(void *addr, size_t length);

// Windows:
//  LPVOID VirtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
//  BOOL VirtualFree(LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType);

static inline void* mmap(void* address, size_t length, int protection, int flags, int fd, off_t offset)
{
    return (void*)syscall6(SYSCALL_MMAP, (intptr_t)address, length, protection, flags, fd, offset);
}


static inline int munmap(void* address, size_t length)
{
  return syscall2(SYSCALL_MUNMAP, (intptr_t)address, length);
}



#ifdef __cplusplus
}
#endif

#endif
