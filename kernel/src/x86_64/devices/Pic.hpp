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

#pragma once

#include "interfaces/IInterruptController.hpp"
#include <cstdint>

// Intel 8259A Programming Interrupt Controller (PIC)
// Ref: https://k.lse.epita.fr/internals/8259a_controller.html

/*
    Legacy PC interrupts:

    IRQ 0 - PIT
    IRQ 1 - Keyboard
    IRQ 2 - Cascaded IRQ 8-15
    IRQ 3 - COM 2 / 4
    IRQ 4 - COM 1 / 3
    IRQ 5 - LPT 2, 3, Sound Card
    IRQ 6 - FDD
    IRQ 7 - LPT 1
    IRQ 8 - RTC
    IRQ 9 - ACPI
    IRQ 10 - SCSI / NIC
    IRQ 11 - SCSI / NIC
    IRQ 12 - Mouse (PS2)
    IRQ 13 - FPU / IPC
    IRQ 14 - Primary ATA
    IRQ 15 - Secondary ATA
*/

class Pic : public IInterruptController
{
public:
    // Initialize the interrupt controller
    std::expected<void, ErrorCode> Initialize() override;

    // Acknowledge an interrupt (End of interrupt / EOI)
    void Acknowledge(int irq) override;

    // Enable the specified interrupt
    void Enable(int irq) override;

    // Disable the specified interrupt
    void Disable(int irq) override;

    // Is the interrupt spurious?
    bool IsSpurious(int irq);

private:
    // Interrupt masks are cached in system memory to save on I/O accesses.
    uint16_t m_mask{0xfffb}; // All IRQs masked by default (except IRQ 2 for cascading interrupts)
};
