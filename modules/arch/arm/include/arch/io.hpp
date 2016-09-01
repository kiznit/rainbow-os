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

#ifndef _RAINBOW_ARCH_ARM_IO_HPP
#define _RAINBOW_ARCH_ARM_IO_HPP

#include <stdint.h>
#include <arch/barrier.hpp>



/*
    Memory Mapped I/O
*/


inline uint8_t mmio_read8(const volatile void* address)
{
    uint8_t value;
    asm volatile("ldrb %1, %0" : "+Qo" (*(volatile uint8_t*)address), "=r" (value));
    read_barrier();
    return value;
}

inline uint16_t mmio_read16(const volatile void* address)
{
    uint16_t value;
    asm volatile("ldrh %1, %0" : "+Q" (*(volatile uint16_t*)address), "=r" (value));
    read_barrier();
    return value;
}

inline uint32_t mmio_read32(const volatile void* address)
{
    uint32_t value;
    asm volatile("ldr %1, %0" : "+Qo" (*(volatile uint32_t*)address), "=r" (value));
    read_barrier();
    return value;
}



inline void mmio_write8(volatile void* address, uint8_t value)
{
    write_barrier();
    asm volatile("strb %1, %0" : "+Qo" (*(volatile uint8_t*)address) : "r" (value));
}

inline void mmio_write16(volatile void* address, uint16_t value)
{
    write_barrier();
    asm volatile("strh %1, %0" : "+Q" (*(volatile uint16_t*)address) : "r" (value));
}

inline void mmio_write32(volatile void* address, uint32_t value)
{
    write_barrier();
    asm volatile("str %1, %0" : "+Qo" (*(volatile uint32_t*)address) : "r" (value));
}




inline uint32_t mmio_read(uintptr_t address)
{
    return mmio_read32((volatile void*)address);
}



inline void mmio_write(uintptr_t address, uint32_t value)
{
    mmio_write32((volatile void*)address, value);
}



#endif
