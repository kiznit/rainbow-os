/*
    Copyright (c) 2022, Thierry Tremblay
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

#include "PageTable.hpp"
#include <cassert>
#include <rainbow/boot.hpp>

extern "C" [[noreturn]] void KernelTrampoline(const BootInfo& bootInfo, const void* kernelEntryPoint);

[[noreturn]] void JumpToKernel(const BootInfo& bootInfo, const void* kernelEntryPoint, PageTable& pageTable)
{
    // There are a number of assumptions here:
    //  1) MMU is enabled by UEFI
    //  2) UEFI is only using TTBR0_EL1 or TTBR0_EL2
    // This means that we are running in low address space and there is no need to relocate a trampoline. We can just jump to the
    // kernel which is in high address space.

    mtl::Write_MAIR_EL1(0xffbb4400);

    if (mtl::GetCurrentEL() == 1)
    {
        auto tcr1 = mtl::Read_TCR_EL1();
        tcr1 &= ~0xFFFF0000;        // Keep TTBR0_EL1 settings
        tcr1 |= 0xB5000000;         // 4KB granules, inner shareable, writeback cache enabled, T1SZ = 16
        tcr1 |= (52 - 4 * 9) << 16; // T1SZ = 52 - 4 translation levels * 9 bits/level
        mtl::Write_TCR_EL1(tcr1);
    }
    else
    {
        // Setup HCR_EL2
        auto hcr = mtl::Read_HCR_EL2();
        hcr |= (1 << 31);  // RW = 1    EL1 Execution state is AArch64.
        hcr &= ~(1 << 27); // TGE = 0   Entry to NS.EL1 is possible
        hcr &= ~(1 << 0);  // VM = 0    Stage 2 MMU disabled
        mtl::Write_HCR_EL2(hcr);

        // Setup VMPIDR_EL2 / VPIDR_EL1
        mtl::Write_VPIDR_EL2(mtl::Read_MIDR_EL1());
        mtl::Write_VMPIDR_EL2(mtl::Read_MPIDR_EL1());

        // Set VMID - Although we are not using stage 2 translation, NS.EL1 still cares about the VMID
        mtl::Write_VTTBR_EL2(0);

        // Make sure EL1 MMU is disabled before setting TCR_EL1
        mtl::Write_SCTLR_EL1(0);

        // Setup TCR_EL1
        const auto tcr_el2 = mtl::Read_TCR_EL2();
        auto tcr1 = tcr_el2 & 0xFFFF;        // Keep TTBR0 settings
        tcr1 |= ((tcr_el2 >> 16) & 7) << 32; // IPS = PS - Physical Address Size
        tcr1 |= 0xB5000000;                  // 4KB granules, inner shareable, writeback cache enabled
        tcr1 |= (52 - 4 * 9) << 16;          // T1SZ = 52 - 4 translation levels * 9 bits/level
        mtl::Write_TCR_EL1(tcr1);

        // Map low memory in EL1
        mtl::Write_TTBR0_EL1(mtl::Read_TTBR0_EL2());

        // Enable MMU at EL1
        uint64_t sctlr_el1{};
        sctlr_el1 |= (1 << 0);  // M = 1        EL1&0 stage 1 address translation enabled.
        sctlr_el1 |= (1 << 2);  // C = 1        Enable Data Cache
        sctlr_el1 |= (1 << 3);  // SA = 1       SP Alignment check enable for EL1
        sctlr_el1 |= (1 << 4);  // SA0 = 1      SP Alignment check enable for EL0
        sctlr_el1 |= (1 << 12); // I = 1        Enable Instruction Cache
        mtl::Write_SCTLR_EL1(sctlr_el1);
    }

    // Map kernel in EL1
    mtl::Write_TTBR1_EL1((uintptr_t)pageTable.GetRaw());

    // See https://stackoverflow.com/questions/58636551/does-aarch64-need-a-dsb-after-creating-a-page-table-entry
    mtl::aarch64_dsb_st(); // Ensure all table entries are visible to the MMU
    mtl::aarch64_isb_sy(); // Ensure the dsb has completed

    KernelTrampoline(bootInfo, kernelEntryPoint);
}
