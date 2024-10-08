/*
    Copyright (c) 2024, Thierry Tremblay
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

#include "Interrupt.hpp"
#include "Cpu.hpp"
#include "acpi/Acpi.hpp"
#include "devices/Apic.hpp"
#include "devices/IoApic.hpp"
#include "devices/Pic.hpp"
#include "interrupt.hpp"

static mtl::unique_ptr<Pic> g_pic;
static mtl::unique_ptr<IoApic> g_ioApic;            // TODO: support more than one I/O APIC
static InterruptHandler g_interruptHandlers[256]{}; // TODO: support multiple handlers per interrupt

// Legacy IRQ interrupts (0-15) can be remapped when using IO APIC
static int g_irqMapping[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

extern "C" void InterruptDispatch(InterruptContext* context)
{
    assert(!mtl::InterruptsEnabled());

    const auto interrupt = context->interrupt;

    if (interrupt >= 32 && interrupt <= 255)
    {
        // If the interrupt source is the PIC, we must check for spurious interrupts
        if ((g_ioApic && Apic::IsSpurious(interrupt)) || (!g_ioApic && g_pic->IsSpurious(interrupt)))
        {
            MTL_LOG(Warning) << "[INTR] Ignoring spurious interrupt " << interrupt;
            return;
        }

        // Dispatch to interrupt controller
        const auto& handler = g_interruptHandlers[interrupt];
        if (handler)
        {
            if (handler.HandleInterrupt(context))
            {
                if (g_ioApic)
                    g_ioApic->Acknowledge(interrupt);
                else
                    g_pic->Acknowledge(interrupt);

                // TODO: yield if we should
                // TODO: do the same when returning from CPU exceptions/faults/traps, not just device interrupts
                // // Interesting thread on how to further improve the logic that determines when to call the scheduler:
                // // https://forum.osdev.org/viewtopic.php?f=1&t=26617
                // if (sched_should_switch)
                // {
                //     g_scheduler.Schedule();
                // }

                return;
            }
        }
    }

    MTL_LOG(Error) << "[INTR] Unhandled interrupt " << interrupt;
}

mtl::expected<void, ErrorCode> InterruptInitialize()
{
    const auto madt = AcpiFindTable<AcpiMadt>("APIC");
    if (!madt)
        MTL_LOG(Warning) << "[INTR] MADT table not found in ACPI";

    // Initialize PIC
    if (!madt || (madt->flags & AcpiMadt::Flag::PcatCompat))
    {
        auto pic = mtl::make_unique<Pic>();
        auto result = pic->Initialize();
        if (pic)
        {
            g_pic = std::move(pic);
        }
        else
            MTL_LOG(Error) << "[INTR] Failed to initialize PIC: " << result.error();
    }

    if (madt)
    {
        bool hasApic{false};
        PhysicalAddress apicAddress = madt->apicAddress;

        const AcpiMadt::Entry* begin = madt->entries;
        const AcpiMadt::Entry* end = (AcpiMadt::Entry*)mtl::AdvancePointer(madt, madt->length);
        for (auto entry = begin; entry < end; entry = mtl::AdvancePointer(entry, entry->length))
        {
            switch (entry->type)
            {
            case AcpiMadt::EntryType::Apic: {
                const auto& info = *(static_cast<const AcpiMadt::Apic*>(entry));
                MTL_LOG(Info) << "[INTR] Found APIC " << info.id;
                hasApic = true;
                break;
            }

            case AcpiMadt::EntryType::IoApic: {
                {
                    const auto& info = *(static_cast<const AcpiMadt::IoApic*>(entry));
                    MTL_LOG(Info) << "[INTR] Found I/O APIC " << info.id << " at address " << mtl::hex(info.address);

                    if (g_ioApic)
                    {
                        MTL_LOG(Warning) << "[INTR] Ignoring I/O APIC beyond the first one";
                        continue;
                    }

                    const auto address = ArchMapSystemMemory(info.address, 1, mtl::PageFlags::MMIO);
                    if (!address)
                    {
                        MTL_LOG(Error) << "[INTR] Failed to map I/O APIC in memory: " << address.error();
                        break;
                    }

                    auto ioApic = mtl::make_unique<IoApic>(address.value());
                    if (!ioApic)
                        return mtl::unexpected(ErrorCode::OutOfMemory);

                    auto result = ioApic->Initialize();
                    if (!result)
                        MTL_LOG(Error) << "[INTR] Error initializing IO APIC: " << (int)result.error();
                    else
                        g_ioApic = std::move(ioApic);
                }
                break;
            }

            case AcpiMadt::EntryType::InterruptOverride: {
                const auto& info = *(static_cast<const AcpiMadt::InterruptOverride*>(entry));
                MTL_LOG(Info) << "[INTR] Found Interrupt Override: bus " << (int)info.bus << ", source " << info.source
                              << ", interrupt " << info.interrupt;
                if (info.bus == AcpiMadt::InterruptOverride::Bus::ISA)
                {
                    if (info.source < 16 && info.interrupt >= 0 && info.interrupt <= 255)
                        g_irqMapping[info.source] = info.interrupt;
                }
                break;
            }

            case AcpiMadt::EntryType::Nmi: {
                const auto& nmi = *(static_cast<const AcpiMadt::Nmi*>(entry));
                MTL_LOG(Info) << "[INTR] Found NMI: CPU " << nmi.processorId;
                break;
            }

            case AcpiMadt::EntryType::ApicAddressOverride: {
                const auto& info = *(static_cast<const AcpiMadt::ApicAddressOverride*>(entry));
                MTL_LOG(Info) << "[INTR] Found APIC address override: " << mtl::hex(info.address);
                apicAddress = info.address;
                break;
            }

            default:
                MTL_LOG(Warning) << "[INTR] Ignoring unknown MADT entry type " << (int)entry->type;
                break;
            }
        }

        if (hasApic)
        {
            const auto address = ArchMapSystemMemory(apicAddress, 1, mtl::PageFlags::MMIO);
            if (address)
            {
                MTL_LOG(Info) << "[INTR] Found APIC at address " << mtl::hex(apicAddress);
                auto apic = mtl::make_unique<Apic>(address.value());
                if (!apic)
                    return mtl::unexpected(ErrorCode::OutOfMemory);

                auto result = apic->Initialize();
                if (!result)
                    MTL_LOG(Error) << "[INTR] Error initializing APIC: " << (int)result.error();
                else
                    CpuSetApic(std::move(apic));
            }
            else
            {
                MTL_LOG(Error) << "[INTR] Failed to map APIC in memory: " << address.error();
            }
        }
    }

    return {};
}

mtl::expected<void, ErrorCode> InterruptRegisterHandler(int interrupt, InterruptHandler handler)
{
    // 0-15 is legacy IRQ range and needs to be remapped at 32 or above
    if (interrupt >= 0 && interrupt <= 15)
    {
        if (g_ioApic)
        {
            const auto newInterrupt = g_irqMapping[interrupt];
            MTL_LOG(Info) << "[INTR] Remapping legacy IRQ " << interrupt << " to interrupt " << newInterrupt;
            interrupt = newInterrupt;
            interrupt = g_ioApic->MapIrqToInterrupt(interrupt);
        }
        else
        {
            interrupt = g_pic->MapIrqToInterrupt(interrupt);
        }
    }

    // 0-31 is reserved for CPU exceptions
    // 32-255 is available
    if (interrupt < 32 || interrupt > 255)
    {
        MTL_LOG(Error) << "[INTR] Can't register handler for invalid interrupt " << interrupt;
        return mtl::unexpected(ErrorCode::InvalidArguments);
    }

    // TODO: support IRQ sharing (i.e. multiple handlers per IRQ)
    if (g_interruptHandlers[interrupt])
    {
        MTL_LOG(Error) << "[INTR] InterruptRegister() - interrupt " << interrupt << " already taken, ignoring request";
        return mtl::unexpected(ErrorCode::Conflict);
    }

    MTL_LOG(Info) << "[INTR] InterruptRegister() - adding handler for interrupt " << interrupt;
    g_interruptHandlers[interrupt] = handler;

    // Enable the interrupt at the controller level
    // TODO: is this the right place to do this?
    if (g_ioApic)
        g_ioApic->Enable(interrupt);
    else
        g_pic->Enable(interrupt);

    return {};
}
