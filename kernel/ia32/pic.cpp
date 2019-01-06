/*
    Copyright (c) 2018, Thierry Tremblay
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

#include "pic.hpp"
#include "../kernel.hpp"


// TODO: need locking
// TODO: cache masks locally


/*
    PIC Reference:
        https://k.lse.epita.fr/internals/8259a_controller.html
*/

#define PIC_MASTER_COMMAND  0x20
#define PIC_MASTER_DATA     0x21
#define PIC_SLAVE_COMMAND   0xA0
#define PIC_SLAVE_DATA      0xA1

// PIC commands
#define PIC_INIT     0x11   // Edge-triggered, ICW4 present
#define PIC_READ_IRR 0x0A
#define PIC_READ_ISR 0x0B
#define PIC_EOI      0x20

/*
    IRQ 0 - PIT
    IRQ 1 - Keyboard
    IRQ 2 - Cacaded IRQ 8-15
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


// Initialize the PICs and remap the interrupts to the specified offset.
// This will also leave all interrupts masked.
void pic_init(int irq_offset)
{
    // ICW1
    io_out_8(PIC_MASTER_COMMAND, PIC_INIT);
    io_wait();
    io_out_8(PIC_SLAVE_COMMAND, PIC_INIT);
    io_wait();

    // ICW2 - IRQ base offsets
    io_out_8(PIC_MASTER_DATA, irq_offset);
    io_wait();
    io_out_8(PIC_SLAVE_DATA, irq_offset + 8);
    io_wait();

    // ICW3
    io_out_8(PIC_MASTER_DATA, 1 << 2);  // Slave is connected to IRQ 2
    io_wait();
    io_out_8(PIC_SLAVE_DATA, 2);        // Slave is connected to IRQ 2
    io_wait();

    // ICW4
    io_out_8(PIC_MASTER_DATA, 1);       // 8086/88 (MCS-80/85) mode
    io_wait();
    io_out_8(PIC_SLAVE_DATA, 1);        // 8086/88 (MCS-80/85) mode
    io_wait();

    // OCW1 - Interrupt masks
    io_out_8(PIC_MASTER_DATA, 0xfb);    // All IRQs masked (except IRQ 2 for slave interrupts)
    io_out_8(PIC_SLAVE_DATA, 0xff);     // All IRQs masked
}



//TODO: Linux caches the current PIC masks to quickly reject masked interrupts
//      This is for performance reasons and a very good idea, do it
int pic_irq_real(int irq)
{
    // We only expect spurious interrupts for IRQ 7 and IRQ 15
    if (irq != 7 && irq != 15)
    {
        return 1;
    }

    int real;
    int mask = 1 << irq;

    if (irq < 8)
    {
        io_out_8(PIC_MASTER_COMMAND, PIC_READ_ISR);
        real = io_in_8(PIC_MASTER_COMMAND) & mask;
        io_out_8(PIC_MASTER_COMMAND, PIC_READ_IRR);
        return real;
    }
    else
    {
        io_out_8(PIC_SLAVE_COMMAND, PIC_READ_ISR);
        real = io_in_8(PIC_SLAVE_COMMAND) & (mask >> 8);
        io_out_8(PIC_SLAVE_COMMAND, PIC_READ_IRR);

        if (!real)
        {
            // Master PIC doesn't know it's a spurious interrupt, so send it an EOI
            io_out_8(PIC_MASTER_COMMAND, PIC_EOI);
        }

        return real;
    }
}



void pic_eoi(int irq)
{
    if (irq >= 8)
    {
        io_out_8(PIC_SLAVE_COMMAND, PIC_EOI);
    }

    io_out_8(PIC_MASTER_COMMAND, PIC_EOI);
}



void pic_disable_irq(int irq)
{
    if (irq >= 0 && irq <= 7)
    {
        uint8_t mask = io_in_8(PIC_MASTER_DATA);
        mask |= (1 << irq);
        io_out_8(PIC_MASTER_DATA, mask);
    }
    else if (irq >=8 && irq <= 15)
    {
        uint8_t mask = io_in_8(PIC_SLAVE_DATA);
        mask |= (1 << (irq - 8));
        io_out_8(PIC_SLAVE_DATA, mask);
    }
}



void pic_enable_irq(int irq)
{
    if (irq >= 0 && irq <= 7)
    {
        uint8_t mask = io_in_8(PIC_MASTER_DATA);
        mask &= ~(1 << irq);
        io_out_8(PIC_MASTER_DATA, mask);
    }
    else if (irq >=8 && irq <= 15)
    {
        uint8_t mask = io_in_8(PIC_SLAVE_DATA);
        mask &= ~(1 << (irq - 8));
        io_out_8(PIC_SLAVE_DATA, mask);
    }
}
