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

typedef intptr_t off_t;

#ifdef __cplusplus
extern "C" {
#endif

// TODO: implement VDSO with ASLR
// TODO: use SYSENTER / SYSCALL (?)


// Parameters to system calls
// ia32:   eax, ebx, ecx, edx, esi, edi, ebp
// x86_64: rax, rdi, rsi, rdx, r10, r8, r9

static inline long syscall1(long function, long arg1)
{
    long result;

#if defined(__i386__)
    asm volatile (
        "int $0x80"
        : "=a"(result)
        : "a"(function),
          "b"(arg1)
        : "memory"
    );
#elif defined(__x86_64__)
    asm volatile (
        "int $0x80"
        : "=a"(result)
        : "a"(function),
          "D"(arg1)
        : "memory"
    );
#endif

    return result;
}


static inline long syscall2(long function, long arg1, long arg2)
{
    long result;

#if defined(__i386__)
    asm volatile (
        "int $0x80"
        : "=a"(result)
        : "a"(function),
          "b"(arg1),
          "c"(arg2)
        : "memory"
    );
#elif defined(__x86_64__)
    asm volatile (
        "int $0x80"
        : "=a"(result)
        : "a"(function),
          "D"(arg1),
          "S"(arg2)
        : "memory"
    );
#endif

    return result;
}


static inline long syscall6(long function, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6)
{
    long result;

#if defined(__i386__)
    register long ebp asm("ebp") = arg6;

    asm volatile (
        "int $0x80"
        : "=a"(result)
        : "a"(function),
          "b"(arg1),
          "c"(arg2),
          "d"(arg3),
          "S"(arg4),
          "D"(arg5),
          "r"(ebp)
        : "memory"
    );
#elif defined(__x86_64__)
    register long r10 asm("r10") = arg4;
    register long r8 asm("r8") = arg5;
    register long r9 asm("r9") = arg6;

    asm volatile (
        "int $0x80"
        : "=a"(result)
        : "a"(function),
          "D"(arg1),
          "S"(arg2),
          "d"(arg3),
          "r"(r10),
          "r"(r8),
          "r"(r9)
        : "memory"
    );
#endif

    return result;
}


static inline long Log(const char* message)
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
