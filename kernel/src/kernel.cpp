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
#include <metal/graphics/SimpleDisplay.hpp>
#include <metal/log.hpp>
#include <rainbow/boot.hpp>

extern std::shared_ptr<mtl::SimpleDisplay> g_earlyDisplay;

void KernelMain(const BootInfo& bootInfo)
{
    MTL_LOG(Info) << "[KRNL] Kernel starting";

    const auto runtimeServices = reinterpret_cast<efi::RuntimeServices*>(bootInfo.uefiRuntimeServices);
    const auto descriptors = reinterpret_cast<const efi::MemoryDescriptor*>(bootInfo.memoryMap);
    MemoryInitialize(runtimeServices, std::vector<efi::MemoryDescriptor>(descriptors, descriptors + bootInfo.memoryMapLength));

    if (g_earlyDisplay)
    {
        g_earlyDisplay->InitializeBackbuffer();
        MTL_LOG(Info) << "[KRNL] Console double-buffering enabled";
    }

    ArchInitialize();

    if (bootInfo.acpiRsdp)
        AcpiInitialize(*reinterpret_cast<const AcpiRsdp*>(bootInfo.acpiRsdp));

    PciInitialize();

    DisplayInitialize();

    AcpiEnable(AcpiInterruptModel::APIC);

    MTL_LOG(Info) << "[KRNL] ACPI Enabled";

    // TODO: at this point we can reclaim AcpiReclaimable memory (?)

    for (;;)
        ;
}
