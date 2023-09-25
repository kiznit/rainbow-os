/*
    Copyright (c) 2023, Thierry Tremblay
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

#pragma once

#include <cstdint>
#include <metal/helpers.hpp>

#define MTL_MRS(NAME)                                                                                                              \
    static inline uint64_t Read_##NAME()                                                                                           \
    {                                                                                                                              \
        uint64_t value;                                                                                                            \
        __asm__ __volatile__("mrs %0, " MTL_STRINGIZE(NAME) : "=r"(value));                                                        \
        return value;                                                                                                              \
    }                                                                                                                              \
                                                                                                                                   \
    static inline void Write_##NAME(uint64_t value)                                                                                \
    {                                                                                                                              \
        asm volatile("msr " MTL_STRINGIZE(NAME) ", %0" : : "r"(value) : "memory");                                                 \
    }

namespace mtl
{
    enum TCR
    {
        EPD0 = 1 << 7, // Translation table walk disable for TTBR0_EL1
    };

    MTL_MRS(CurrentEL);

    MTL_MRS(SPSEL);

    MTL_MRS(CNTFRQ_EL0); // EL1 Timer counter frequency
    MTL_MRS(CNTPCT_EL0); // EL1 Timer physical counter
    MTL_MRS(CNTVCT_EL0); // EL1 Timer virtual counter
    MTL_MRS(ELR_EL1)
    MTL_MRS(ESR_EL1)
    MTL_MRS(FAR_EL1)
    MTL_MRS(ID_AA64MMFR0_EL1)
    MTL_MRS(ID_AA64MMFR1_EL1)
    MTL_MRS(MAIR_EL1);
    MTL_MRS(MIDR_EL1);
    MTL_MRS(MPIDR_EL1);
    MTL_MRS(SCTLR_EL1);
    MTL_MRS(SP_EL1);
    MTL_MRS(SPSR_EL1);
    MTL_MRS(TCR_EL1);
    MTL_MRS(TPIDR_EL1);
    MTL_MRS(TTBR0_EL1);
    MTL_MRS(TTBR1_EL1);
    MTL_MRS(VBAR_EL1);

    MTL_MRS(ELR_EL2)
    MTL_MRS(ESR_EL2)
    MTL_MRS(FAR_EL2)
    MTL_MRS(HCR_EL2);
    MTL_MRS(MAIR_EL2);
    MTL_MRS(SCTLR_EL2);
    MTL_MRS(SPSR_EL2);
    MTL_MRS(TCR_EL2);
    MTL_MRS(TTBR0_EL2);
    MTL_MRS(VBAR_EL2);
    MTL_MRS(VPIDR_EL2);
    MTL_MRS(VMPIDR_EL2);
    MTL_MRS(VTTBR_EL2);

    static inline int GetCurrentEL()
    {
        return (Read_CurrentEL() >> 2) & 3;
    }

    // Data Memory Barrier
    static inline void aarch64_dmb_sy()
    {
        __asm__ __volatile__("dmb sy" : : : "memory");
    }

    // Data Synchronization Barrier
    static inline void aarch64_dsb_sy()
    {
        __asm__ __volatile__("dsb sy" : : : "memory");
    }
    static inline void aarch64_dsb_st()
    {
        __asm__ __volatile__("dsb st" : : : "memory");
    }
    static inline void aarch64_dsb_ishst()
    {
        __asm__ __volatile__("dsb ishst" : : : "memory");
    }
    static inline void aarch64_dsb_ish()
    {
        __asm__ __volatile__("dsb ish" : : : "memory");
    }

    // Instruction Synchronization Barrier
    static inline void aarch64_isb_sy()
    {
        __asm__ __volatile__("isb sy" : : : "memory");
    }

    // Data Cache Clean by virtual address
    static inline void aarch64_dc_civac(const void* address)
    {
        __asm__ __volatile__("dc civac, %0" : : "r"(address) : "memory");
    }

    // Invalidate TLB by virtual address
    static inline void aarch64_tlbi_vae1(const void* address)
    {
        __asm__ __volatile__("tlbi vae1, %0" : : "r"(address) : "memory");
    }

    // Invalidate all TLBs
    static inline void aarch64_tlbi_vmalle1()
    {
        __asm__ __volatile__("tlbi vmalle1" : : : "memory");
    }

    ///////////////////////////////////////////////////////////////////////////
    // Interrupts
    ///////////////////////////////////////////////////////////////////////////

    // Enable interrupts for the current CPU
    static inline void EnableInterrupts()
    {
        asm volatile("msr daifclr, #0x3" ::: "memory");
    }

    // Disable interrupts for the current CPU
    static inline void DisableInterrupts()
    {
        asm volatile("msr daifset, #0x3" ::: "memory");
    }

    // Are interrupts enabled for the current CPU?
    static inline bool InterruptsEnabled()
    {
        uint64_t flags;
        asm volatile("mrs %0, daif" : "=r"(flags)::"memory");
        return (flags >> 6) & 3 ? false : true;
    }

    ///////////////////////////////////////////////////////////////////////////
    // CPU
    ///////////////////////////////////////////////////////////////////////////

    static inline void CpuPause()
    {
        asm volatile("yield" ::: "memory");
    }
} // namespace mtl
