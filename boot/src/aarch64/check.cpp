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

    */

    bool ok = false;

    // The UEFI specification says we can be in EL1 or EL2 mode.
    // TODO: implement support for EL2 (have trampoline set TTBL1_EL2 and kernel switch to EL1?)
    const auto el = mtl::GetCurrentEL();
    if (el != 1)
    {
        MTL_LOG(Error) << "Current execution mode (EL) is " << el << ", expected 1";
        ok = false;
    }

    assert(mtl::Read_MAIR_EL1() == 0xffbb4400);

    return true;
}