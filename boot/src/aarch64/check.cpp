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

#include <metal/arch.hpp>
#include <metal/log.hpp>

bool CheckArch()
{
    /*
        UEFI Specification says:

            - Unaligned access must be enabled.
            - Use the highest 64 bit non secure privilege level available; Non-secure EL2 (Hyp) or Non-secure EL1(Kernel).
            - The MMU is enabled and any RAM defined by the UEFI memory map is identity mapped (virtual address equals physical
       address). The mappings to other regions are undefined and may vary from implementation to implementation
            - The core will be configured as follows:
                - MMU enabled
                - Instruction and Data caches enabled
                - Little endian mode
                - Stack Alignment Enforced
                - NOT Top Byte Ignored
                - Valid Physical Address Space
                - 4K Translation Granule
            - MAIR:
                - Attr 0: 0x00 - EFI_MEMORY_UC
                - Attr 1: 0x44 - EFI_MEMORY_WC
                - Attr 2: 0xbb - EFI_MEMORY_WT
                - Attr 3: 0xff - EFI_MEMORY_WB

        QEMU Virt starts in EL1 with:
            ID_AA64MMFR0_EL1: 0000000000001122
            ID_AA64MMFR1_EL1: 0000000000000000
            SCTLR_EL1: 0000000000c5183d
                SPAN   : 1 - The value of PSTATE.PAN is left unchanged on taking an exception to EL1.
                EIS    : 1 - The taking of an exception to EL1 is a context synchronizing event.
                nTWE   : 1 - This control does not cause any instructions to be trapped.
                nTWI   : 1 - This control does not cause any instructions to be trapped.
                I      : 1 - Stage 1 instruction access Cacheability control, for accesses at EL0 and EL1.
                EOS    : 1 - An exception return from EL1 is a context synchronizing event.
                CP15BEN: 1 - EL0 using AArch32: EL0 execution of the CP15DMB, CP15DSB, and CP15ISB instructions is enabled.
                SA0    : 1 - SP Alignment check enable for EL0.
                SA     : 1 - SP Alignment check enable.
                C      : 1 - Stage 1 Cacheability control, for data accesses.
                M      : 1 - EL1&0 stage 1 address translation enabled.
            TCR_EL1  : 0000000280803518
                IPS : 010 - Intermediate Physical Size - 40 bits, 1 TB
                TG1 : 10  - Granule, 4KB
                EPD1: 1   - Disable TTBR1_EL1
                SH0 : 11  - Inner shareable
                ORGN: 01  - Outer cacheability
                IRGN: 01  - Inner cacheability
                T0SZ: 011000 - Size offset = 2 ^ (64 - T0SZ) = 2 ^ 40
            TTBR0_EL1: 000000023ffff000

        Raspberry Pi 3 starts in EL2 with:
            ID_AA64MMFR0_EL1: 0000000000001122
            ID_AA64MMFR1_EL1: 0000000000000000
            SCTLR_EL2: 0000000030c5183d
            TCR_EL2  : 0000000080823518
                RES1: 1   - Reserved
                RES1: 1   - Reserved
                PS  : 010 - Physical Address Size = 40 bit, 1 TB
                SH0 : 11  - Inner shareable
                ORGN: 01  - Outer cacheability
                IRGN: 01  - Inner cacheability
                T0SZ: 011000 - Size offset = 2 ^ (64 - T0SZ) = 2 ^ 40
            TTBR0_EL2: 000000003b3f7000
    */

    bool ok = true;

    // The UEFI specification says we can be in EL1 or EL2 mode.
    const auto el = mtl::GetCurrentEL();

    MTL_LOG(Debug) << "CurrentEL: " << el;

    if (el == 1)
    {
        MTL_LOG(Debug) << "SCTLR_EL1: " << mtl::hex(mtl::Read_SCTLR_EL1());
        MTL_LOG(Debug) << "TCR_EL1  : " << mtl::hex(mtl::Read_TCR_EL1());
        MTL_LOG(Debug) << "TTBR0_EL1: " << mtl::hex(mtl::Read_TTBR0_EL1());
    }
    else if (el == 2)
    {
        assert(mtl::Read_MAIR_EL2() == 0xffbb4400);
        MTL_LOG(Debug) << "SCTLR_EL2: " << mtl::hex(mtl::Read_SCTLR_EL2());
        MTL_LOG(Debug) << "TCR_EL2  : " << mtl::hex(mtl::Read_TCR_EL2());
        MTL_LOG(Debug) << "TTBR0_EL2: " << mtl::hex(mtl::Read_TTBR0_EL2());
    }
    else
    {
        MTL_LOG(Error) << "Current execution mode is EL" << el << ", needs EL1 or EL2";
        ok = false;
    }

    MTL_LOG(Debug) << "ID_AA64MMFR0_EL1: " << mtl::hex(mtl::Read_ID_AA64MMFR0_EL1());
    MTL_LOG(Debug) << "ID_AA64MMFR1_EL1: " << mtl::hex(mtl::Read_ID_AA64MMFR1_EL1());

    return ok;
}