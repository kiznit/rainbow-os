/*
    Copyright (c) 2017, Thierry Tremblay
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


#include <stddef.h>
#include <rainbow/arch.hpp>


extern char* PERIPHERAL_BASE;


#define GPIO_BASE                           (PERIPHERAL_BASE + 0x00200000)  // GPIO Base Address
#define GPIO_GPFSEL0                        (GPIO_BASE + 0x00000000)        // GPIO Function Select 0
#define GPIO_GPFSEL1                        (GPIO_BASE + 0x00000004)        // GPIO Function Select 1
#define GPIO_GPFSEL2                        (GPIO_BASE + 0x00000008)        // GPIO Function Select 2
#define GPIO_GPFSEL3                        (GPIO_BASE + 0x0000000C)        // GPIO Function Select 3
#define GPIO_GPFSEL4                        (GPIO_BASE + 0x00000010)        // GPIO Function Select 4
#define GPIO_GPFSEL5                        (GPIO_BASE + 0x00000014)        // GPIO Function Select 5
#define GPIO_GPSET0                         (GPIO_BASE + 0x0000001C)        // GPIO Pin Output Set 0
#define GPIO_GPSET1                         (GPIO_BASE + 0x00000020)        // GPIO Pin Output Set 1
#define GPIO_GPCLR0                         (GPIO_BASE + 0x00000028)        // GPIO Pin Output Clear 0
#define GPIO_GPCLR1                         (GPIO_BASE + 0x0000002C)        // GPIO Pin Output Clear 1
#define GPIO_GPLEV0                         (GPIO_BASE + 0x00000034)        // GPIO Pin Level 0
#define GPIO_GPLEV1                         (GPIO_BASE + 0x00000038)        // GPIO Pin Level 1
#define GPIO_GPEDS0                         (GPIO_BASE + 0x00000040)        // GPIO Pin Event Detect Status 0
#define GPIO_GPEDS1                         (GPIO_BASE + 0x00000044)        // GPIO Pin Event Detect Status 1
#define GPIO_GPREN0                         (GPIO_BASE + 0x0000004C)        // GPIO Pin Rising Edge Detect Enable 0
#define GPIO_GPREN1                         (GPIO_BASE + 0x00000050)        // GPIO Pin Rising Edge Detect Enable 1
#define GPIO_GPFEN0                         (GPIO_BASE + 0x00000058)        // GPIO Pin Falling Edge Detect Enable 0
#define GPIO_GPFEN1                         (GPIO_BASE + 0x0000005C)        // GPIO Pin Falling Edge Detect Enable 1
#define GPIO_GPHEN0                         (GPIO_BASE + 0x00000064)        // GPIO Pin High Detect Enable 0
#define GPIO_GPHEN1                         (GPIO_BASE + 0x00000068)        // GPIO Pin High Detect Enable 1
#define GPIO_GPLEN0                         (GPIO_BASE + 0x00000070)        // GPIO Pin Low Detect Enable 0
#define GPIO_GPLEN1                         (GPIO_BASE + 0x00000074)        // GPIO Pin Low Detect Enable 1
#define GPIO_GPAREN0                        (GPIO_BASE + 0x0000007C)        // GPIO Pin Async. Rising Edge Detect 0
#define GPIO_GPAREN1                        (GPIO_BASE + 0x00000080)        // GPIO Pin Async. Rising Edge Detect 1
#define GPIO_GPAFEN0                        (GPIO_BASE + 0x00000088)        // GPIO Pin Async. Falling Edge Detect 0
#define GPIO_GPAFEN1                        (GPIO_BASE + 0x0000008C)        // GPIO Pin Async. Falling Edge Detect 1
#define GPIO_GPPUD                          (GPIO_BASE + 0x00000094)        // GPIO Pin Pull-up/down Enable
#define GPIO_GPPUDCLK0                      (GPIO_BASE + 0x00000098)        // GPIO Pin Pull-up/down Enable Clock 0
#define GPIO_GPPUDCLK1                      (GPIO_BASE + 0x0000009C)        // GPIO Pin Pull-up/down Enable Clock 1
#define GPIO_TEST                           (GPIO_BASE + 0x000000B0)        // GPIO Test

// PL011 UART
#define UART0_BASE      (GPIO_BASE + 0x00001000)
#define UART0_DR        (UART0_BASE + 0x00)
#define UART0_RSRECR    (UART0_BASE + 0x04)
#define UART0_FR        (UART0_BASE + 0x18)
#define UART0_ILPR      (UART0_BASE + 0x20)
#define UART0_IBRD      (UART0_BASE + 0x24)
#define UART0_FBRD      (UART0_BASE + 0x28)
#define UART0_LCRH      (UART0_BASE + 0x2C)
#define UART0_CR        (UART0_BASE + 0x30)
#define UART0_IFLS      (UART0_BASE + 0x34)
#define UART0_IMSC      (UART0_BASE + 0x38)
#define UART0_RIS       (UART0_BASE + 0x3C)
#define UART0_MIS       (UART0_BASE + 0x40)
#define UART0_ICR       (UART0_BASE + 0x44)
#define UART0_DMACR     (UART0_BASE + 0x48)
#define UART0_ITCR      (UART0_BASE + 0x80)
#define UART0_ITIP      (UART0_BASE + 0x84)
#define UART0_ITOP      (UART0_BASE + 0x88)
#define UART0_TDR       (UART0_BASE + 0x8C)

// Mini UART
#define AUX_ENABLES     (PERIPHERAL_BASE + 0x00215004)
#define AUX_MU_IO_REG   (PERIPHERAL_BASE + 0x00215040)
#define AUX_MU_IER_REG  (PERIPHERAL_BASE + 0x00215044)
#define AUX_MU_IIR_REG  (PERIPHERAL_BASE + 0x00215048)
#define AUX_MU_LCR_REG  (PERIPHERAL_BASE + 0x0021504C)
#define AUX_MU_MCR_REG  (PERIPHERAL_BASE + 0x00215050)
#define AUX_MU_LSR_REG  (PERIPHERAL_BASE + 0x00215054)
#define AUX_MU_MSR_REG  (PERIPHERAL_BASE + 0x00215058)
#define AUX_MU_SCRATCH  (PERIPHERAL_BASE + 0x0021505C)
#define AUX_MU_CNTL_REG (PERIPHERAL_BASE + 0x00215060)
#define AUX_MU_STAT_REG (PERIPHERAL_BASE + 0x00215064)
#define AUX_MU_BAUD_REG (PERIPHERAL_BASE + 0x00215068)


extern "C" void cpu_delay(int unused);


// Wait at least 150 GPU cycles (and not 150 CPU cycles)
static void gpio_delay()
{
    for (int i = 0; i != 150; ++i)
    {
        cpu_delay(i);
    }
}



class RaspberryPL011Uart
{
public:

    void Initialize()
    {
        unsigned int ra;

        // Disable UART 0
        mmio_write32(UART0_CR, 0);

        // Map UART0 (alt function 0) to GPIO pins 14 and 15
        ra = mmio_read32(GPIO_GPFSEL1);
        ra &= ~(7 << 12);   //gpio14
        ra |= 4 << 12;      //alt0
        ra &= ~(7 << 15);   //gpio15
        ra |= 4 << 15;      //alt0
        mmio_write32(GPIO_GPFSEL1, ra);
        mmio_write32(GPIO_GPPUD, 0);
        gpio_delay();
        mmio_write32(GPIO_GPPUDCLK0, 3 << 14);
        gpio_delay();
        mmio_write32(GPIO_GPPUDCLK0, 0);

        // Clear pending interrupts
        mmio_write32(UART0_ICR, 0x7FF);

        // Baud rate
        // Divider = UART_CLOCK / (16 * Baud)
        // Fraction = (Fraction part * 64) + 0.5

        // Raspberry 2: UART CLOCK = 3000000 (3MHz)
        // Divider = 3000000 / (16 * 115200) = 1.627    --> 1
        // Fraction = (.627 * 64) + 0.5 = 40.6          --> 40
        //mmio_write32(UART0_IBRD, 1);
        //mmio_write32(UART0_FBRD, 40);

        // Raspberry 3: UART_CLOCK = 48000000 (48 MHz)
        // Divider = 48000000 / (16 * 115200) = 26.041766667  --> 26
        // Fraction = (.041766667 * 64) + 0.5 = 3.1666667     --> 3
        mmio_write32(UART0_IBRD, 26);
        mmio_write32(UART0_FBRD, 3);

        // Enable FIFO, 8-N-1
        mmio_write32(UART0_LCRH, 0x70);

        // Mask all interrupts
        mmio_write32(UART0_IMSC, 0x7F2);

        // Enable UART0 (receive + transmit)
        mmio_write32(UART0_CR, 0x301);
    }


    void putc(unsigned int c)
    {
        while(1)
        {
            if ((mmio_read32(UART0_FR) & 0x20)==0)
                break;
        }

        mmio_write32(UART0_DR, c);

        if (c == '\n')
        {
            putc('\r');
        }
    }


    unsigned int getc()
    {
        while(1)
        {
            if ((mmio_read32(UART0_FR) & 0x10)==0)
                break;
        }

        return mmio_read32(UART0_DR);
    }
};



class RaspberryMiniUart
{
public:

    void Initialize()
    {
        unsigned int ra;

        mmio_write32(AUX_ENABLES, 1);
        mmio_write32(AUX_MU_IER_REG, 0);
        mmio_write32(AUX_MU_CNTL_REG, 0);
        mmio_write32(AUX_MU_LCR_REG, 3);
        mmio_write32(AUX_MU_MCR_REG, 0);
        mmio_write32(AUX_MU_IER_REG, 0);
        mmio_write32(AUX_MU_IIR_REG, 0xC6);
        mmio_write32(AUX_MU_BAUD_REG, 270);

        // Map Mini UART (alt function 5) to GPIO pins 14 and 15
        ra = mmio_read32(GPIO_GPFSEL1);
        ra &= ~(7<<12); //gpio14
        ra |= 2<<12;    //alt5
        ra &= ~(7<<15); //gpio15
        ra |= 2<<15;    //alt5
        mmio_write32(GPIO_GPFSEL1, ra);
        mmio_write32(GPIO_GPPUD, 0);
        gpio_delay();
        mmio_write32(GPIO_GPPUDCLK0, (1<<14)|(1<<15));
        gpio_delay();
        mmio_write32(GPIO_GPPUDCLK0, 0);

        mmio_write32(AUX_MU_CNTL_REG, 3);
    }


    void putc(unsigned int c)
    {
        while(1)
        {
            if (mmio_read32(AUX_MU_LSR_REG) & 0x20)
                break;
        }

        mmio_write32(AUX_MU_IO_REG, c);

        if (c == '\n')
        {
            putc('\r');
        }
    }


    unsigned int getc()
    {
        while(1)
        {
            if (mmio_read32(AUX_MU_LSR_REG) & 0x01)
                break;
        }

        return mmio_read32(AUX_MU_IO_REG) & 0xFF;
    }


    void flush()
    {
        for (;;)
        {
            if ((mmio_read32(AUX_MU_LSR_REG) & 0x100)==0)
                break;
        }
    }
};


//static RaspberryPL011Uart uart;
static RaspberryMiniUart uart;



void libc_initialize()
{
    uart.Initialize();
}



extern "C" int _libc_print(const char* string)
{
    size_t length = 0;
    for (const char* p = string; *p; ++p, ++length)
    {
        uart.putc(*p);
    }

    return length;
}



extern "C" int getchar()
{
    return uart.getc();
}



extern "C" void abort()
{
    getchar();

    //todo: reset system? how?

    for (;;)
    {
        //todo: asm("cli; hlt");
    }
}
