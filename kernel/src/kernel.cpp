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

#include "acpi/acpi.hpp"
#include "arch.hpp"
#include "display.hpp"
#include "memory.hpp"
#include "pci.hpp"
#include "uefi.hpp"
#include <metal/log.hpp>
#include <rainbow/boot.hpp>

void KernelMain(const BootInfo& bootInfo)
{
    ArchInitialize();

    MTL_LOG(Info) << "[KRNL] Rainbow OS kernel starting";

    // Make sure to call UEFI's SetVirtualMemoryMap() while we have the UEFI boot services still mapped in the lower 4 GB.
    // This is to work around buggy runtime firmware that call into boot services during a call to SetVirtualMemoryMap().
    UefiInitialize(*reinterpret_cast<efi::SystemTable*>(bootInfo.uefiSystemTable));

    // Once UEFI is initialized, it is save to release boot services code and data.
    MemoryInitialize();

    if (auto rsdp = UefiFindAcpiRsdp())
        AcpiInitialize(*rsdp);

    PciInitialize();

    DisplayInitialize();

    AcpiEnable(AcpiInterruptModel::APIC);

    MTL_LOG(Info) << "[KRNL] ACPI Enabled";

    // TODO: at this point we can reclaim AcpiReclaimable memory (?)

    for (;;)
        ;
}
