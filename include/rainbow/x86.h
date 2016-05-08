/*
    Copyright (c) 2016, Thierry Tremblay
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

#ifndef _RAINBOW_RAINBOW_X86_H
#define _RAINBOW_RAINBOW_X86_H

#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef uint64_t physaddr_t;



// Page mapping flags (12 bits)
#define PAGE_PRESENT        0x001
#define PAGE_WRITE          0x002
#define PAGE_USER           0x004
#define PAGE_WRITE_THROUGH  0x008
#define PAGE_CACHE_DISABLE  0x010
#define PAGE_ACCESSED       0x020
#define PAGE_DIRTY          0x040
#define PAGE_LARGE          0x080
#define PAGE_GLOBAL         0x100

#define PAGE_ALLOCATED      0x200   // Page was allocated (vmm_alloc)
#define PAGE_RESERVED_1     0x400
#define PAGE_RESERVED_2     0x800


#define CR0_PG  (1 << 31)
#define CR4_PAE (1 << 5)


/*
 * Volatile isn't enough to prevent the compiler from reordering the
 * read/write functions for the control registers and messing everything up.
 * A memory clobber would solve the problem, but would prevent reordering of
 * all loads stores around it, which can hurt performance. The solution is to
 * use a variable and mimic reads and writes to it to enforce serialization
 */
extern unsigned long __x86_force_order;



inline uintptr_t x86_read_cr0()
{
    uintptr_t value;
    asm ("mov %%cr0, %0" : "=r"(value), "=m" (__x86_force_order));
    return value;
}



inline uintptr_t x86_read_cr3()
{
    uintptr_t value;
    asm ("mov %%cr3, %0" : "=r"(value), "=m" (__x86_force_order));
    return value;
}



inline uintptr_t x86_read_cr4()
{
    uintptr_t value;
    asm ("mov %%cr4, %0" : "=r"(value), "=m" (__x86_force_order));
    return value;
}



inline void x86_write_cr0(uintptr_t value)
{
    asm volatile ("mov %0, %%cr0" : : "r"(value), "m" (__x86_force_order));
}



inline void x86_write_cr3(uintptr_t value)
{
    asm volatile ("mov %0, %%cr3" : : "r"(value), "m" (__x86_force_order));
}



inline void x86_write_cr4(uintptr_t value)
{
    asm volatile ("mov %0, %%cr4" : : "r"(value), "m" (__x86_force_order));
}



inline void x86_invlpg(uintptr_t virtualAddress)
{
    asm volatile ("invlpg (%0)" : : "r"(virtualAddress) : "memory");
}



#ifdef __cplusplus
}
#endif

#endif
