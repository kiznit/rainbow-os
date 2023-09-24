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

#include "interrupt.hpp"
#include "Cpu.hpp"
#include "Task.hpp"
#include "acpi/Acpi.hpp"
#include "devices/Apic.hpp"
#include "devices/IoApic.hpp"
#include "devices/Pic.hpp"
#include <metal/log.hpp>

/*
    x86 CPU exceptions
    0   #DE - Divide Error                  16  #MF - Floating-Point Error
    1   #DB - Debug                         17  #AC - Alignment Check
    2         NMI                           18  #MC - Machine Check
    3   #BP - Breakpoint                    19  #XM/#XF - SIMD Floating-Point Error
    4   #OF - Overflow                      20  #VE - Virtualization Exception
    5   #BR - BOUND Range Exceeded          21  - Reserved -
    6   #UD - Invalid Opcode                22  - Reserved -
    7   #NM - Device Not Available          23  - Reserved -
    8   #DF - Double Fault                  24  - Reserved -
    9   - Reserved -                        25  - Reserved -
    10  #TS - Invalid TSS                   26  - Reserved -
    11  #NP - Segment Not Present           27  - Reserved -
    12  #SS - Stack Fault                   28  #HV - Hypervisor Injection Exception (AMD only?)
    13  #GP - General Protection            29  #VC - VMM Communication Exception (AMD only?)
    14  #PF - Page Fault                    30  #SX - Security Exception (AMD only?)
    15  - Reserved -                        31   - Reserved -
    The following CPU exceptions will push an error code: 8, 10-14, 17, 30.
*/

static void LogException(const char* exception, const InterruptContext* context)
{
    MTL_LOG(Debug) << "CPU EXCEPTION: " << exception << ", error " << mtl::hex(context->error) << ", task "
                   << Cpu::GetCurrentTask()->GetId();

    MTL_LOG(Debug) << "    rax: " << mtl::hex(context->rax) << "    rbp: " << mtl::hex(context->rbp)
                   << "    r8 : " << mtl::hex(context->r8) << "    r12   : " << mtl::hex(context->r12);
    MTL_LOG(Debug) << "    rbx: " << mtl::hex(context->rbx) << "    rsi: " << mtl::hex(context->rsi)
                   << "    r9 : " << mtl::hex(context->r9) << "    r13   : " << mtl::hex(context->r13);
    MTL_LOG(Debug) << "    rcx: " << mtl::hex(context->rcx) << "    rdi: " << mtl::hex(context->rdi)
                   << "    r10: " << mtl::hex(context->r10) << "    r14   : " << mtl::hex(context->r14);
    MTL_LOG(Debug) << "    rdx: " << mtl::hex(context->rdx) << "    rsp: " << mtl::hex(context->rsp)
                   << "    r11: " << mtl::hex(context->r11) << "    r15   : " << mtl::hex(context->r15);
    MTL_LOG(Debug) << "    cs : " << mtl::hex(context->cs) << "    rip: " << mtl::hex(context->rip)
                   << "    ss : " << mtl::hex(context->ss) << "    rflags: " << mtl::hex(context->rflags);

    const auto stack = (uint64_t*)context->rsp;
    for (int i = 0; i != 10; ++i)
    {
        MTL_LOG(Debug) << "    stack[" << i << "]: " << mtl::hex(stack[i]);
    }
}

#define UNHANDLED_EXCEPTION(vector, name)                                                                                          \
    extern "C" void Exception##name(InterruptContext* context)                                                                     \
    {                                                                                                                              \
        LogException(#name, context);                                                                                              \
        MTL_LOG(Fatal) << "Unhandled CPU exception: " << mtl::hex<uint8_t>(vector) << " (" #name ")";                              \
        std::abort();                                                                                                              \
    }

UNHANDLED_EXCEPTION(0, DivideError)
UNHANDLED_EXCEPTION(1, Debug)
UNHANDLED_EXCEPTION(2, Nmi)
UNHANDLED_EXCEPTION(3, Breakpoint)
UNHANDLED_EXCEPTION(4, Overflow)
UNHANDLED_EXCEPTION(5, BoundRangeExceeded)
UNHANDLED_EXCEPTION(6, InvalidOpcode)
UNHANDLED_EXCEPTION(8, DoubleFault)
UNHANDLED_EXCEPTION(10, InvalidTss)
UNHANDLED_EXCEPTION(11, StackSegment)
UNHANDLED_EXCEPTION(12, Stack)
UNHANDLED_EXCEPTION(13, General)
UNHANDLED_EXCEPTION(16, Fpu)
UNHANDLED_EXCEPTION(17, Alignment)
UNHANDLED_EXCEPTION(18, MachineCheck)
UNHANDLED_EXCEPTION(19, Simd)

extern "C" void ExceptionPageFault(InterruptContext* context)
{
    const auto address = (void*)mtl::Read_CR2();
    LogException("PageFault", context);
    MTL_LOG(Fatal) << "Unhandled CPU exception: 0e (PageFault), address " << address;
    std::abort();
}

static std::unique_ptr<Pic> g_pic;
static std::unique_ptr<IoApic> g_ioApic;                                 // TODO: support more than one I/O APIC
static IInterruptHandler* g_interruptHandlers[256 - kLegacyIrqOffset]{}; // TODO: support multiple handlers per interrupt

// Legacy IRQ interrupts (0-15) need to be remapped to kLegacyIrqOffset
static int g_irqMapping[16] = {kLegacyIrqOffset + 0,  kLegacyIrqOffset + 1,  kLegacyIrqOffset + 2,  kLegacyIrqOffset + 3,
                               kLegacyIrqOffset + 4,  kLegacyIrqOffset + 5,  kLegacyIrqOffset + 6,  kLegacyIrqOffset + 7,
                               kLegacyIrqOffset + 8,  kLegacyIrqOffset + 9,  kLegacyIrqOffset + 10, kLegacyIrqOffset + 11,
                               kLegacyIrqOffset + 12, kLegacyIrqOffset + 13, kLegacyIrqOffset + 14, kLegacyIrqOffset + 15};

std::expected<void, ErrorCode> InterruptInitialize(const Acpi* acpi)
{
    auto madt = acpi ? acpi->FindTable<AcpiMadt>("APIC") : nullptr;
    if (!madt)
        MTL_LOG(Warning) << "[INTR] MADT table not found in ACPI";

    // Initialize PIC
    if (!madt || (madt->flags & AcpiMadt::Flag::PcatCompat))
    {
        auto pic = std::make_unique<Pic>();
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
                    if (g_ioApic)
                    {
                        MTL_LOG(Warning) << "[INTR] Ignoring I/O APIC beyond the first one";
                        continue;
                    }

                    const auto& info = *(static_cast<const AcpiMadt::IoApic*>(entry));
                    MTL_LOG(Info) << "[INTR] Found I/O APIC " << info.id << " at address " << mtl::hex(info.address);

                    const auto address = ArchMapSystemMemory(info.address, 1, mtl::PageFlags::MMIO);
                    if (!address)
                    {
                        MTL_LOG(Error) << "[INTR] Failed to map I/O APIC in memory: " << address.error();
                        break;
                    }

                    auto ioApic = std::make_unique<IoApic>(address.value());
                    if (!ioApic)
                        return std::unexpected(ErrorCode::OutOfMemory);

                    auto result = ioApic->Initialize();
                    if (!result)
                        MTL_LOG(Error) << "[INT] Error initializing IO APIC: " << (int)result.error();
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
                        g_irqMapping[info.source] = info.interrupt + kLegacyIrqOffset;
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
                auto apic = std::make_unique<Apic>(address.value());
                if (!apic)
                    return std::unexpected(ErrorCode::OutOfMemory);

                auto result = apic->Initialize();
                if (!result)
                    MTL_LOG(Error) << "[INTR] Error initializing APIC: " << (int)result.error();
                else
                    Cpu::GetCurrent().SetApic(std::move(apic));
            }
            else
            {
                MTL_LOG(Error) << "[INTR] Failed to map APIC in memory: " << address.error();
            }
        }
    }

    return {};
}

std::expected<void, ErrorCode> InterruptRegister(int interrupt, IInterruptHandler& handler)
{
    // 0-15 is legacy IRQ range and available
    // 16-31 is reserved for CPU exceptions
    // 32-255 is available
    if ((interrupt >= 16 && interrupt < 32) || interrupt > 255)
    {
        MTL_LOG(Error) << "[INTR] Can't register handler for invalid interrupt " << interrupt;
        return std::unexpected(ErrorCode::InvalidArguments);
    }

    // If the interrupt is under 16, we assume it is a legacy IRQ and remap it.
    // TODO: this is ugly, but it is x86_64 specific.
    if (interrupt < 16)
    {
        const auto newInterrupt = g_irqMapping[interrupt];
        MTL_LOG(Info) << "[INTR] Remapping legacy IRQ" << interrupt << " to interrupt " << newInterrupt;
        interrupt = newInterrupt;
    }

    // TODO: support IRQ sharing (i.e. multiple handlers per IRQ)
    if (g_interruptHandlers[interrupt])
    {
        MTL_LOG(Error) << "[INTR] InterruptRegister() - interrupt " << interrupt << " already taken, ignoring request";
        return std::unexpected(ErrorCode::Conflict);
    }

    g_interruptHandlers[interrupt] = &handler;

    // Enable the interrupt at the controller level
    // TODO: is this the right place to do that?

    if (g_ioApic)
        g_ioApic->Enable(interrupt - 32);
    else
        g_pic->Enable(interrupt - 32);

    return {};
}

extern "C" void InterruptDispatch(InterruptContext* context)
{
    assert(!mtl::InterruptsEnabled());
    assert(context->interrupt >= 32 && context->interrupt <= 255);

    // If the interrupt source is the PIC, we must check for spurious interrupts
    if (!g_ioApic)
    {
        if (g_pic->IsSpurious(context->interrupt - kLegacyIrqOffset))
        {
            MTL_LOG(Warning) << "[INTR] Ignoring spurious interrupt " << context->interrupt;
            return;
        }
    }

    // Dispatch to interrupt controller
    const auto& handler = g_interruptHandlers[context->interrupt];
    if (handler)
    {
        if (handler->HandleInterrupt(context))
        {
            if (g_ioApic)
                g_ioApic->Acknowledge(context->interrupt - kLegacyIrqOffset);
            else
                g_pic->Acknowledge(context->interrupt - kLegacyIrqOffset);

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

    MTL_LOG(Error) << "[INTR] Unhandled interrupt " << context->interrupt;
}
