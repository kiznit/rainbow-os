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

#include "Pic.hpp"
#include <metal/arch.hpp>

using mtl::x86_inb;
using mtl::x86_io_delay;
using mtl::x86_outb;

/*
    PIC Reference:
        https://k.lse.epita.fr/internals/8259a_controller.html
*/

// TODO: need locking (?)

constexpr auto kInterruptOffset = 32;

constexpr auto PIC_MASTER_COMMAND = 0x20;
constexpr auto PIC_MASTER_DATA = 0x21;
constexpr auto PIC_SLAVE_COMMAND = 0xA0;
constexpr auto PIC_SLAVE_DATA = 0xA1;

// PIC commands
constexpr auto PIC_INIT = 0x11; // Edge-triggered, ICW4 present
constexpr auto PIC_SELECT_IRR = 0x0A;
constexpr auto PIC_SELECT_ISR = 0x0B;
constexpr auto PIC_EOI = 0x20;

std::expected<void, ErrorCode> Pic::Initialize()
{
    // kInterruptOffset must be a multiple of 8.
    static_assert(!(kInterruptOffset & 7));

    // ICW1
    x86_outb(PIC_MASTER_COMMAND, PIC_INIT);
    x86_io_delay();
    x86_outb(PIC_SLAVE_COMMAND, PIC_INIT);
    x86_io_delay();

    // ICW2 - IRQ base offsets
    x86_outb(PIC_MASTER_DATA, kInterruptOffset);
    x86_io_delay();
    x86_outb(PIC_SLAVE_DATA, kInterruptOffset + 8);
    x86_io_delay();

    // ICW3
    x86_outb(PIC_MASTER_DATA, 1 << 2); // Slave is connected to IRQ 2
    x86_io_delay();
    x86_outb(PIC_SLAVE_DATA, 2); // Slave is connected to IRQ 2
    x86_io_delay();

    // ICW4
    x86_outb(PIC_MASTER_DATA, 1); // 8086/88 (MCS-80/85) mode
    x86_io_delay();
    x86_outb(PIC_SLAVE_DATA, 1); // 8086/88 (MCS-80/85) mode
    x86_io_delay();

    // OCW1 - Interrupt masks
    x86_outb(PIC_MASTER_DATA, m_mask);
    x86_outb(PIC_SLAVE_DATA, m_mask >> 8);

    return {};
}

void Pic::Acknowledge(int irq)
{
    if (irq < 0 || irq > 15)
    {
        MTL_LOG(Warning) << "[PIC] Acknowledge() - irq out of range: " << irq;
        return;
    }

    if (irq >= 8)
        x86_outb(PIC_SLAVE_COMMAND, PIC_EOI);

    x86_outb(PIC_MASTER_COMMAND, PIC_EOI);
}

void Pic::Enable(int irq)
{
    if (irq < 0 || irq > 15 || irq == 2)
    {
        MTL_LOG(Warning) << "[PIC] Enable() - irq out of range: " << irq;
        return;
    }

    m_mask &= ~(1 << irq);

    if (irq < 8)
        x86_outb(PIC_MASTER_DATA, m_mask);
    else
        x86_outb(PIC_SLAVE_DATA, m_mask >> 8);
}

void Pic::Disable(int irq)
{
    if (irq < 0 || irq > 15 || irq == 2)
    {
        MTL_LOG(Warning) << "[PIC] Disable() - irq out of range: " << irq;
        return;
    }

    m_mask |= (1 << irq);

    if (irq < 8)
        x86_outb(PIC_MASTER_DATA, m_mask);
    else
        x86_outb(PIC_SLAVE_DATA, m_mask >> 8);
}

// Lot of info on spurious interrupts:
// https://lore.kernel.org/all/200403211858.07445.hpj@urpla.net/T/
bool Pic::IsSpurious(int irq)
{
    // Spurious interrupts are only expected on IRQ 7 and IRQ 15.
    if (irq == 7)
    {
        x86_outb(PIC_MASTER_COMMAND, PIC_SELECT_ISR);
        auto real = x86_inb(PIC_MASTER_COMMAND) & (1 << 7);
        x86_outb(PIC_MASTER_COMMAND, PIC_SELECT_IRR);
        return !real;
    }
    else if (irq == 15)
    {
        x86_outb(PIC_SLAVE_COMMAND, PIC_SELECT_ISR);
        auto real = x86_inb(PIC_SLAVE_COMMAND) & (1 << 7);
        x86_outb(PIC_SLAVE_COMMAND, PIC_SELECT_IRR);

        // Master PIC doesn't know it's a spurious interrupt, so send it an EOI
        if (!real)
            x86_outb(PIC_MASTER_COMMAND, PIC_EOI);

        return !real;
    }
    else
    {
        return false;
    }
}