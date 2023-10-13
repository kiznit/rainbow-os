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

#include "Cpu.hpp"
#include "Task.hpp"
#include "acpi/Acpi.hpp"
#include "devices/GicCpuInterface.hpp"
#include "devices/GicDistributor.hpp"
#include "interfaces/IInterruptHandler.hpp"
#include <array>

namespace
{
    std::unique_ptr<GicDistributor> g_gicd;         // TODO: support more than one GICD? Is that possible?
    IInterruptHandler* g_interruptHandlers[1024]{}; // TODO: do we need that many?

} // namespace

// Interrupt dispatch
extern "C" void Exception_EL1h_SPx_IRQ(InterruptContext* context)
{
    const auto iar = Cpu::GetGicCpuInterface()->ReadIAR();
    const auto cpu = (iar >> 10) & 7;
    const auto interrupt = (iar & 0x3FF);

    if (interrupt == 1023)
    {
        MTL_LOG(Warning) << "[INTR] Ignoring spurious interrupt " << interrupt;
        return;
    }

    const auto& handler = g_interruptHandlers[interrupt];
    if (handler)
    {
        if (handler->HandleInterrupt(context))
        {
            g_gicd->Acknowledge(interrupt);
            return;
        }
    }

    MTL_LOG(Error) << "[INTR] Unhandled interrupt " << interrupt << " from CPU " << cpu;
    (void)context; // TODO
}

namespace InterruptSystem
{
    std::expected<void, ErrorCode> Initialize()
    {
        auto madt = Acpi::FindTable<AcpiMadt>("APIC");
        if (!madt)
        {
            MTL_LOG(Warning) << "[INTR] MADT table not found in ACPI";
            return {};
        }

        const AcpiMadt::Entry* begin = madt->entries;
        const AcpiMadt::Entry* end = (AcpiMadt::Entry*)mtl::AdvancePointer(madt, madt->length);
        for (auto entry = begin; entry < end; entry = mtl::AdvancePointer(entry, entry->length))
        {
            switch (entry->type)
            {
            case AcpiMadt::EntryType::GicCpuInterface: {
                const auto& info = *(static_cast<const AcpiMadt::GicCpuInterface*>(entry));
                MTL_LOG(Info) << "[INTR] Found GIC CPU Interface " << info.id << " at address " << mtl::hex(info.address);

                // TODO: we assume we are running on CPU 0, we don't know that
                if (info.id != 0)
                    continue;

                auto result = GicCpuInterface::Create(info);
                if (!result)
                {
                    MTL_LOG(Error) << "[INTR] Error initializing GIC CPU Interface: " << (int)result.error();
                    continue;
                }

                Cpu::GetCurrent().SetGicCpuInterface(std::move(*result));
                break;
            }

            case AcpiMadt::EntryType::GicDistributor: {
                const auto& info = *(static_cast<const AcpiMadt::GicDistributor*>(entry));
                MTL_LOG(Info) << "[INTR] Found GIC Distributor " << info.id << " at address " << mtl::hex(info.address)
                              << ", version is " << info.version;
                if (g_gicd)
                {
                    MTL_LOG(Warning) << "[INTR] Ignoring GIC Distributor beyond the first one";
                    continue;
                }

                auto result = GicDistributor::Create(info);
                if (!result)
                {
                    MTL_LOG(Error) << "[INTR] Error initializing GIC Distributor: " << (int)result.error();
                    return std::unexpected(result.error());
                }

                g_gicd = std::move(*result);
                break;
            }

            case AcpiMadt::EntryType::GicMsiFrame: {
                const auto& info = *(static_cast<const AcpiMadt::GicMsiFrame*>(entry));
                MTL_LOG(Info) << "[INTR] Found GIC MSI Frame " << info.id << " at address " << mtl::hex(info.address);
                break;
            }

            default:
                MTL_LOG(Warning) << "[INTR] Ignoring unknown MADT entry type " << (int)entry->type;
                break;
            }
        }

        return {};
    }

    std::expected<void, ErrorCode> RegisterHandler(int interrupt, IInterruptHandler& handler)
    {
        // TODO: check if lower interrupt numbers are reserved
        // TODO: is it appropriate to have handlers for high numbers (1021,1022,1023)?
        if (interrupt < 0 || interrupt >= std::ssize(g_interruptHandlers))
        {
            MTL_LOG(Error) << "[INTR] Can't register handler for invalid interrupt " << interrupt;
            return std::unexpected(ErrorCode::InvalidArguments);
        }

        // TODO: support IRQ sharing (i.e. multiple handlers per IRQ)
        if (g_interruptHandlers[interrupt])
        {
            MTL_LOG(Error) << "[INTR] InterruptRegister() - interrupt " << interrupt << " already taken, ignoring request";
            return std::unexpected(ErrorCode::Conflict);
        }

        MTL_LOG(Info) << "[INTR] InterruptRegister() - adding handler for interrupt " << interrupt;
        g_interruptHandlers[interrupt] = &handler;

        // Enable the interrupt at the controller level
        // TODO: is this the right place to do that?
        g_gicd->SetGroup(interrupt, 0);
        g_gicd->SetPriority(interrupt, 0);
        g_gicd->SetTargetCpu(interrupt, 0x01);
        g_gicd->SetTrigger(interrupt, GicDistributor::Trigger::Edge);
        g_gicd->Acknowledge(interrupt); // Clear any pending interrupt
        g_gicd->Enable(interrupt);

        return {};
    }

} // namespace InterruptSystem
