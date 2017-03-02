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

#ifndef RAINBOW_ARCH_ARM_HPP
#define RAINBOW_ARCH_ARM_HPP


// Models are a combinaison of implementor and part number.
#define ARM_CPU_MODEL_ARM1176   0x4100b760
#define ARM_CPU_MODEL_CORTEXA7  0x4100c070
#define ARM_CPU_MODEL_CORTEXA53 0x4100d030
#define ARM_CPU_MODEL_MASK      0xff00fff0


inline unsigned arm_cpuid_id()
{
    uint32_t value;

    // Retrieve the processor's Main ID Register (MIDR)
#if defined(__arm__)
    asm("mrc p15,0,%0,c0,c0,0" : "=r"(value) : : "cc");
#elif defined(__aarch64__)
    asm("mrs %0, MIDR_EL1" : "=r"(value) : : "cc");
#endif

    return value;
}


inline unsigned arm_cpuid_model()
{
    return arm_cpuid_id() & ARM_CPU_MODEL_MASK;
}



typedef uint32_t physaddr_t;

#define MEMORY_PAGE_SHIFT 12
#define MEMORY_PAGE_SIZE 4096

#define MEMORY_LARGE_PAGE_SHIFT 16
#define MEMORY_LARGE_PAGE_SIZE 65536


#define read_barrier()  __asm__ __volatile__ ("" : : : "memory")
#define write_barrier() __asm__ __volatile__ ("" : : : "memory")


inline uint32_t mmio_read32(const volatile void* address)
{
    uint32_t value;
    asm volatile("ldr %0, %1" : "=r" (value) : "m" (*(volatile uint32_t*)address));
    read_barrier();
    return value;
}


inline void mmio_write32(volatile void* address, uint32_t value)
{
    write_barrier();
    asm volatile("str %0, %1" : : "r" (value), "m" (*(volatile uint32_t*)address));
}


#endif
