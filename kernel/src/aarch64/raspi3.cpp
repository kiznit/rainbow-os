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

#include "raspi3.hpp"
#include "arch.hpp"

struct GpioRegisters
{
    uint32_t GPFSEL0;       // GPIO Function Select 0
    uint32_t GPFSEL1;       // GPIO Function Select 1
    uint32_t GPFSEL2;       // GPIO Function Select 2
    uint32_t GPFSEL3;       // GPIO Function Select 3
    uint32_t GPFSEL4;       // GPIO Function Select 4
    uint32_t GPFSEL5;       // GPIO Function Select 5
    uint32_t GPSET0;        // GPIO Pin Output Set 0
    uint32_t reserved0;     // Reserved
    uint32_t GPSET1;        // GPIO Pin Output Set 1
    uint32_t reserved1;     // Reserved
    uint32_t GPCLR0;        // GPIO Pin Output Clear 0
    uint32_t GPCLR1;        // GPIO Pin Output Clear 1
    uint32_t reserved2;     // Reserved
    uint32_t GPLEV0;        // GPIO Pin Level 0
    uint32_t GPLEV1;        // GPIO Pin Level 1
    uint32_t reserved3;     // Reserved
    uint32_t GPEDS0;        // GPIO Pin Event Detect Status 0
    uint32_t GPEDS1;        // GPIO Pin Event Detect Status 1
    uint32_t reserved4;     // Reserved
    uint32_t GPREN0;        // GPIO Pin Rising Edge Detect Enable 0
    uint32_t GPREN1;        // GPIO Pin Rising Edge Detect Enable 1
    uint32_t reserved5;     // Reserved
    uint32_t GPFEN0;        // GPIO Pin Falling Edge Detect Enable 0
    uint32_t GPFEN1;        // GPIO Pin Falling Edge Detect Enable 1
    uint32_t reserved6;     // Reserved
    uint32_t GPHEN0;        // GPIO Pin High Detect Enable 0
    uint32_t GPHEN1;        // GPIO Pin High Detect Enable 1
    uint32_t reserved7;     // Reserved
    uint32_t GPLEN0;        // GPIO Pin Low Detect Enable 0
    uint32_t GPLEN1;        // GPIO Pin Low Detect Enable 1
    uint32_t reserved8;     // Reserved
    uint32_t GPAREN0;       // GPIO Pin Async. Rising Edge Detect 0
    uint32_t GPAREN1;       // GPIO Pin Async. Rising Edge Detect 1
    uint32_t reserved9;     // Reserved
    uint32_t GPAFEN0;       // GPIO Pin Async. Falling Edge Detect 0
    uint32_t GPAFEN1;       // GPIO Pin Async. Falling Edge Detect 1
    uint32_t reserved10;    // Reserved
    uint32_t GPPUD;         // GPIO Pin Async. Falling Edge Detect 0
    uint32_t GPPUDCLK0;     // GPIO Pin Pull-up/down Enable Clock 0
    uint32_t GPPUDCLK1;     // GPIO Pin Pull-up/down Enable Clock 1
    uint32_t reserved11[4]; // Reserved
    uint32_t TEST;          // GPIO Test
};

static_assert(sizeof(GpioRegisters) == 0xB4);

static void GpioDelay()
{
    // TODO: we need to wait at least 150 GPU cycles (and not 150 CPU cycles, which is what this is doing right now)
    for (int i = 0; i != 150; ++i)
        asm volatile("nop");
}

void MapUartToGPIO()
{
    const auto gpio = (volatile GpioRegisters*)ArchMapSystemMemory(kGpioBase, 1, mtl::PageFlags::MMIO).value();

    // Map UART0 (alt function 0) to GPIO pins 14 and 15
    auto ra = gpio->GPFSEL1;
    ra &= ~(7 << 12); // gpio14
    ra |= 4 << 12;    // alt0
    ra &= ~(7 << 15); // gpio15
    ra |= 4 << 15;    // alt0
    gpio->GPFSEL1 = ra;

    gpio->GPPUD = 0;
    GpioDelay();
    gpio->GPPUDCLK0 = 3 << 14;
    GpioDelay();
    gpio->GPPUDCLK0 = 0;
}
