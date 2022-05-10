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

#pragma once

#include <cstdint>

namespace mtl
{
    enum class Msr : unsigned int
    {
        IA32_MTRRCAP = 0x000000FE,

        IA32_SYSENTER_CS = 0x00000174,
        IA32_SYSENTER_ESP = 0x00000175,
        IA32_SYSENTER_EIP = 0x00000176,

        // Variable Range MTRRs
        IA32_MTRR_PHYSBASE0 = 0x00000200,
        IA32_MTRR_PHYSMASK0 = 0x00000201,
        IA32_MTRR_PHYSBASE1 = 0x00000202,
        IA32_MTRR_PHYSMASK1 = 0x00000203,
        IA32_MTRR_PHYSBASE2 = 0x00000204,
        IA32_MTRR_PHYSMASK2 = 0x00000205,
        IA32_MTRR_PHYSBASE3 = 0x00000206,
        IA32_MTRR_PHYSMASK3 = 0x00000207,
        IA32_MTRR_PHYSBASE4 = 0x00000208,
        IA32_MTRR_PHYSMASK4 = 0x00000209,
        IA32_MTRR_PHYSBASE5 = 0x0000020A,
        IA32_MTRR_PHYSMASK5 = 0x0000020B,
        IA32_MTRR_PHYSBASE6 = 0x0000020C,
        IA32_MTRR_PHYSMASK6 = 0x0000020D,
        IA32_MTRR_PHYSBASE7 = 0x0000020E,
        IA32_MTRR_PHYSMASK7 = 0x0000020F,
        // ... up to IA32_MTRRCAP::VCNT

        // Fixed Range MTRRs
        IA32_MTRR_FIX64K_00000 = 0x00000250,
        IA32_MTRR_FIX16K_80000 = 0x00000258,
        IA32_MTRR_FIX16K_A0000 = 0x00000259,
        IA32_MTRR_FIX4K_C0000 = 0x00000268,
        IA32_MTRR_FIX4K_C8000 = 0x00000269,
        IA32_MTRR_FIX4K_D0000 = 0x0000026A,
        IA32_MTRR_FIX4K_D8000 = 0x0000026B,
        IA32_MTRR_FIX4K_E0000 = 0x0000026C,
        IA32_MTRR_FIX4K_E8000 = 0x0000026D,
        IA32_MTRR_FIX4K_F0000 = 0x0000026E,
        IA32_MTRR_FIX4K_F8000 = 0x0000026F,

        IA32_PAT = 0x00000277,

        IA32_MTRR_DEF_TYPE = 0x000002FF,

        // x86-64 specific MSRs
        IA32_EFER = 0xc0000080,          // extended feature register
        IA32_STAR = 0xc0000081,          // Legacy mode SYSCALL target
        IA32_LSTAR = 0xc0000082,         // Long mode SYSCALL target
        IA32_CSTAR = 0xc0000083,         // Compat mode SYSCALL target
        IA32_FMASK = 0xc0000084,         // EFLAGS mask for SYSCALL
        IA32_FS_BASE = 0xc0000100,       // 64bit FS base
        IA32_GS_BASE = 0xc0000101,       // 64bit GS base
        IA32_KERNEL_GSBASE = 0xc0000102, // SwapGS GS shadow

        IA32_TSC_AUX = 0xc0000103, // Auxiliary TSC
    };

    // IA32_EFER bits
    constexpr uint64_t IA32_EFER_SCE = (1 << 0);    // SYSCALL / SYSRET
    constexpr uint64_t IA32_EFER_LME = (1 << 8);    // Long mode enable
    constexpr uint64_t IA32_EFER_LMA = (1 << 10);   // Long mode active (read-only)
    constexpr uint64_t IA32_EFER_NX = (1 << 11);    // No execute enable
    constexpr uint64_t IA32_EFER_SVME = (1 << 12);  // Enable virtualization
    constexpr uint64_t IA32_EFER_LMSLE = (1 << 13); // Long mode segment limit enable
    constexpr uint64_t IA32_EFER_FFXSR = (1 << 14); // Enable fast FXSAVE/FXRSTOR

    static inline uint64_t x86_read_msr(Msr msr)
    {
        uint32_t low, high;
        asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
        return (uint64_t(high) << 32) | low;
    }

    static inline void x86_write_msr(Msr msr, uint64_t value)
    {
        const uint32_t low = value & 0xFFFFFFFF;
        const uint32_t high = value >> 32;

        asm volatile("wrmsr" : : "c"(msr), "a"(low), "d"(high) : "memory");
    }

} // namespace mtl
