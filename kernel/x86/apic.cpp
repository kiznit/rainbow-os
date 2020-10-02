/*
    Copyright (c) 2020, Thierry Tremblay
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

#include "apic.hpp"
#include "acpi.hpp"
#include "cpu.hpp"
#include <metal/helpers.hpp>
#include <metal/log.hpp>
#include <metal/x86/memory.hpp>
#include <kernel/vmm.hpp>


static physaddr_t s_localApicAddress;
void* s_apic;


void apic_init()
{
    auto madt = (const Acpi::Madt*)acpi_find_table(acpi_signature("APIC"));
    if (!madt) return;

    s_localApicAddress = (uintptr_t)madt->localApicAddress;

    for (auto entry = (const Acpi::Madt::Entry*)(madt + 1); (uintptr_t)entry < (uintptr_t)madt + madt->length; entry = advance_pointer(entry, entry->length))
    {
        // TODO: handle other entry types
        switch (entry->type)
        {
            case 0:
            {
                auto localApic = (const Acpi::Madt::LocalApic*)entry;
                Log("    Local APIC %d, CPU %d, flags %d\n", localApic->id, localApic->processorId, localApic->flags);

                // CPU detection is done my enumerating APICs. This doesn't seem very intuitive but seems to be the way to go about it.
                if (g_cpuCount < MAX_CPU)
                {
                    if (localApic->flags & (Acpi::Madt::LocalApic::FLAG_ENABLED | Acpi::Madt::LocalApic::FLAG_ONLINE_CAPABLE))
                    {
                        Cpu& cpu = g_cpus[g_cpuCount++];
                        cpu.id = localApic->processorId;
                        cpu.apicId = localApic->id;
                        cpu.enabled = (localApic->flags & Acpi::Madt::LocalApic::FLAG_ENABLED) ? true : false;
                        cpu.bootstrap = false; // BSP to be detected once we map the LAPIC, see below
                    }
                }

                break;
            }

            case 1:
            {
                auto ioApic = (const Acpi::Madt::IoApic*)entry;
                Log("    I/O APIC %d at address %x\n", ioApic->id, ioApic->address);
                break;
            }

            case 2:
            {
                auto interruptOverride = (const Acpi::Madt::InterruptOverride*)entry;
                Log("    Interrupt override bus %d, source %d, interrupt %d, flags %x\n", interruptOverride->bus, interruptOverride->source, interruptOverride->interrupt, interruptOverride->flags);
                break;
            }

            case 4:
            {
                auto nmi = (const Acpi::Madt::Nmi*)entry;
                Log("    NMI cpu %d, lint %d, flags %x\n", nmi->processorId, nmi->lint, nmi->flags);
                break;
            }

            case 5:
            {
                auto addressOverride = (const Acpi::Madt::LocalApicAddressOverride*)entry;
                s_localApicAddress = addressOverride->address;
                break;
            }

            default:
            {
                Log("    Unknown entry %d\n", entry->type);
            }
        }
    }

    Log("    Local APIC address: %X\n", s_localApicAddress);

    if (!s_localApicAddress) { return; }

    // Select UC memory type
    s_apic = vmm_map_pages(s_localApicAddress, 1, PAGE_WRITE_THROUGH | PAGE_CACHE_DISABLE);
    Log("    Local APIC mapped to %p\n", s_apic);

    // Find which CPU is the BSP
    const auto localApicId = apic_read(APIC_ID);
    for (int i = 0; i != g_cpuCount; ++i)
    {
        if (g_cpus[i].apicId == localApicId)
        {
            g_cpus[i].bootstrap = true;
        }
    }
}
