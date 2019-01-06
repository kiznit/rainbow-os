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

#ifndef _RAINBOW_LIBK_X86_HPP
#define _RAINBOW_LIBK_X86_HPP

#include <stdint.h>


// Memory
typedef uint64_t physaddr_t;

#define MEMORY_PAGE_SHIFT 12
#define MEMORY_PAGE_SIZE 4096

// Here we assume PAE where large pages are 2 MB. Non-PAE large pages are 4 MB.
#define MEMORY_LARGE_PAGE_SHIFT 21
#define MEMORY_LARGE_PAGE_SIZE (2*1024*1024)

// Huge pages are 1 GB
#define MEMORY_HUGE_PAGE_SHIFT 30
#define MEMORY_HUGE_PAGE_SIZE (1024*1024*1024)  // 1 GB

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



// EFLAGS
#define X86_EFLAGS_IF 0x00000200



/*
 * Volatile isn't enough to prevent the compiler from reordering the
 * read/write functions for the control registers and messing everything up.
 * A memory clobber would solve the problem, but would prevent reordering of
 * all loads stores around it, which can hurt performance. The solution is to
 * use a variable and mimic reads and writes to it to enforce serialization
 */

extern unsigned long __force_order;


static inline uintptr_t x86_get_cr0()
{
    uintptr_t value;
    asm ("mov %%cr0, %0" : "=r"(value), "=m" (__force_order));
    return value;
}


static inline void x86_set_cr0(uintptr_t value)
{
    asm volatile ("mov %0, %%cr0" : : "r"(value), "m" (__force_order));
}


static inline uintptr_t x86_get_cr3()
{
    uintptr_t physicalAddress;
    asm ("mov %%cr3, %0" : "=r"(physicalAddress), "=m" (__force_order));
    return physicalAddress;
}


static inline void x86_set_cr3(uintptr_t physicalAddress)
{
    asm volatile ("mov %0, %%cr3" : : "r"(physicalAddress), "m" (__force_order));
}


static inline uintptr_t x86_get_cr4()
{
    uintptr_t value;
    asm ("mov %%cr4, %0" : "=r"(value), "=m" (__force_order));
    return value;
}


static inline void x86_set_cr4(uintptr_t value)
{
    asm volatile ("mov %0, %%cr4" : : "r"(value), "m" (__force_order));
}


// Interrupts
// Enable interrupts for the current CPU
static inline void interrupt_enable()
{
    asm volatile ("sti":::"memory");
}


// Disable interrupts for the current CPU
static inline void interrupt_disable()
{
    asm volatile ("cli":::"memory");
}


// Are interrupts enabled for the current CPU?
static inline int interrupt_enabled()
{
    intptr_t flags;
    asm volatile ("pushf; pop %0;" : "=r"(flags));
    return flags & X86_EFLAGS_IF;
}


// I/O
static inline void io_out_8(uint16_t port, uint8_t value)
{
    asm volatile ("outb %1, %0" : : "dN" (port), "a" (value));
}


static inline void io_out_16(uint16_t port, uint16_t value)
{
    asm volatile ("outw %1, %0" : : "dN" (port), "a" (value));
}


static inline void io_out_32(uint16_t port, uint32_t value)
{
    asm volatile ("outl %1, %0" : : "dN" (port), "a" (value));
}


static inline uint8_t io_in_8(uint16_t port)
{
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}


static inline uint16_t io_in_16(uint16_t port)
{
    uint16_t ret;
    asm volatile ("inw %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}


static inline uint32_t io_in_32(uint16_t port)
{
    uint32_t ret;
    asm volatile ("inl %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}


static inline void io_wait()
{
    /* Port 0x80 is used for 'checkpoints' during POST. */
    /* The Linux kernel seems to think it is free for use :-/ */
    asm volatile ("outb %al, $0x80");
}


#endif
