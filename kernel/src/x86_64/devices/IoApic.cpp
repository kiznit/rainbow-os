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

#include "IoApic.hpp"
#include "InterruptHandler.hpp"
#include "interrupt.hpp"
#include <metal/log.hpp>

constexpr auto kInterruptOffset = 32;

IoApic::IoApic(void* address)
    : m_ioregsel(reinterpret_cast<volatile Register*>(address)),
      m_iowin(reinterpret_cast<volatile uint32_t*>(mtl::AdvancePointer(address, 0x10))),
      m_id((Read32(Register::IOAPICID) >> 24) & 0xF), m_version(Read32(Register::IOAPICVER) & 0xFF),
      m_interruptCount(((Read32(Register::IOAPICVER) >> 16) & 0xFF) + 1),
      m_arbitrationId((Read32(Register::IOAPICARB) >> 24) & 0x0F)
{
}

std::expected<void, ErrorCode> IoApic::Initialize()
{
    MTL_LOG(Info) << "[APIC] I/O APIC initialized: IOREGSEL = " << m_ioregsel << ", IOWIN = " << m_iowin;
    MTL_LOG(Info) << "    ID            : " << m_id;
    MTL_LOG(Info) << "    Version       : " << m_version;
    MTL_LOG(Info) << "    Interrupts    : " << m_interruptCount;
    MTL_LOG(Info) << "    Arbitration id: " << m_arbitrationId;

    return {};
}

std::expected<void, ErrorCode> IoApic::RegisterHandler(int interrupt, IInterruptHandler* handler)
{
    if (interrupt < 0 || interrupt >= m_interruptCount)
        return std::unexpected(ErrorCode::InvalidArguments);

    if (m_handlers[interrupt])
    {
        MTL_LOG(Error) << "[APIC] RegisterHandler() - interrupt " << interrupt << " already taken, ignoring request";
        return std::unexpected(ErrorCode::Conflict);
    }

    m_handlers[interrupt] = handler;

    // TODO: The trick to programming these things is setting the polarity and trigger mode. For the IRQ 0..15 interrupts, set it to
    // 'edge triggered, active high' mode (both and zero). For the PCI A..D interrupts, set it to 'level triggered, active low'
    // (both and one). At least that's what works on my motherboard.

    uint64_t redirection = interrupt + kInterruptOffset;
    assert(redirection >= 0x10 && redirection <= 0xFE); // Valid range for interrupt vector is 0x10..0xFE
    redirection |= (1 << 16);                           // Disable interrupt
    Write64(GetRegister(interrupt), redirection);

    return {};
}

void IoApic::Acknowledge(int interrupt)
{
    if (interrupt < 0 || interrupt >= m_interruptCount)
    {
        MTL_LOG(Warning) << "[APIC] Acknowledge() - interrupt out of range: " << interrupt;
        return;
    }

    // TODO
}

void IoApic::Enable(int interrupt)
{
    if (interrupt < 0 || interrupt >= m_interruptCount)
    {
        MTL_LOG(Warning) << "[APIC] Enable() - interrupt out of range: " << interrupt;
        return;
    }

    const auto reg = GetRegister(interrupt);
    auto value = Read32(reg);
    value &= ~(1 << 16);
    Write32(reg, value);
}

void IoApic::Disable(int interrupt)
{
    if (interrupt < 0 || interrupt >= m_interruptCount)
    {
        MTL_LOG(Warning) << "[APIC] Disable() - interrupt out of range: " << interrupt;
        return;
    }

    const auto reg = GetRegister(interrupt);
    auto value = Read32(reg);
    value |= (1 << 16);
    Write32(reg, value);
}

void IoApic::HandleInterrupt(InterruptContext* context)
{
    const auto interrupt = (int)context->interrupt;
    if (interrupt < 0 || interrupt >= m_interruptCount)
    {
        MTL_LOG(Warning) << "[APIC] HandleInterrupt() - interrupt out of range: " << interrupt;
        return;
    }

    // Dispatch to interrupt handler
    const auto handler = m_handlers[interrupt];
    if (handler && handler->HandleInterrupt(context))
    {
        Acknowledge(context->interrupt);
        return;
    }
    else
    {
        MTL_LOG(Error) << "[APIC] Unhandled interrupt " << interrupt;
    }
}
