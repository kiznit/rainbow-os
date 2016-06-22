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

#ifndef _RAINBOW_KERNEL_MMIO_HPP
#define _RAINBOW_KERNEL_MMIO_HPP

#include <stdint.h>



/*
    Memory Mapped I/O - volatile is used as a memory barrier
*/


inline uint32_t mmio_read(uintptr_t address)
{
    // TODO: memory barrier
    //  1) ensure all writes are completed (memory mapped or not!)
    //  2) ensure memory mapped reads are not re-ordered ... ?

    //__sync_synchronize();
    asm volatile ("" : : : "memory");
    return *(volatile uint32_t*)address;
}



inline void mmio_write(uintptr_t address, uint32_t value)
{
    // TODO: memory barriers
    //  1) ensure all writes are completed (memory mapped or not!)

    //__sync_synchronize();

    asm volatile ("" : : : "memory");
    *(volatile uint32_t*)address = value;
}



#endif
