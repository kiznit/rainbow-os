#include <kernel/kernel.hpp>
#include <arch/io.hpp>
#include <stddef.h>

#include "raspi.hpp"


static void cpu_delay()
{
    asm volatile("nop");
}



// Wait at least 150 GPU cycles (and not 150 CPU cycles)
static void gpio_delay()
{
    for (int i = 0; i != 150; ++i)
    {
        cpu_delay();
    }
}



#define GET32(x) mmio_read32((void*)x)
#define PUT32(x,y) mmio_write32((void*)x, y)



class RaspberryUart
{
public:

    void Initialize()
    {
        unsigned int ra;

        PUT32(UART0_CR, 0);

        // TODO: this GPIO init business is only required once, do it at CPU init time
        ra = GET32(GPIO_GPFSEL1);
        ra &= ~(7 << 12);   //gpio14
        ra |= 4 << 12;      //alt0
        ra &= ~(7 << 15);   //gpio15
        ra |= 4 << 15;      //alt0
        PUT32(GPIO_GPFSEL1, ra);

        PUT32(GPIO_GPPUD, 0);
        gpio_delay();
        PUT32(GPIO_GPPUDCLK0, 3 << 14);
        gpio_delay();
        PUT32(GPIO_GPPUDCLK0, 0);

        PUT32(UART0_ICR, 0x7FF);
        PUT32(UART0_IBRD, 1);
        PUT32(UART0_FBRD, 40);
        PUT32(UART0_LCRH, 0x70);
        PUT32(UART0_CR, 0x301);
    }


    void putc(unsigned int c)
    {
        while(1)
        {
            if ((GET32(UART0_FR) & 0x20)==0)
                break;
        }

        PUT32(UART0_DR, c);
    }


    unsigned int getc()
    {
        while(1)
        {
            if ((GET32(UART0_FR) & 0x10)==0)
                break;
        }

        return GET32(UART0_DR);
    }
};




static bool console_initialized = false;
static RaspberryUart uart;

extern "C" int _libc_print(const char* string, size_t length)
{
    if (!console_initialized)
    {
        uart.Initialize();
        console_initialized = true;
    }

    for (size_t i = 0; i != length; ++i)
    {
        uart.putc(string[i]);
    }

    return length;
}



extern "C" void abort()
{
    //todo: kernel panic
    //todo: disable interrupts
    cpu_halt();
}
