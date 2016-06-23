#include <kernel/kernel.hpp>
#include <arch/io.hpp>
#include <stddef.h>


/*
    todo: this code is Raspberry specific
*/

#if defined(__ARM_ARCH_6ZK__)
// Raspberry
#define RASPI_IO_BASE 0x20000000
#else
// Raspberry 2, 3
#define RASPI_IO_BASE 0x3F000000
#endif

#define GPIO_BASE       (RASPI_IO_BASE + 0x200000)

#define GPIO_FSEL1      (GPIO_BASE + 0x04)
#define GPIO_SET0       (GPIO_BASE + 0x1C)
#define GPIO_CLR0       (GPIO_BASE + 0x28)
#define GPIO_PUD        (GPIO_BASE + 0x94)
#define GPIO_PUDCLK0    (GPIO_BASE + 0x98)


#define UART0_BASE      (GPIO_BASE + 0x1000)

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

        ra = GET32(GPIO_FSEL1);
        ra &= ~(7 << 12);   //gpio14
        ra |= 4 << 12;      //alt0
        ra &= ~(7 << 15);   //gpio15
        ra |= 4 << 15;      //alt0
        PUT32(GPIO_FSEL1, ra);

        PUT32(GPIO_PUD, 0);
        gpio_delay();
        PUT32(GPIO_PUDCLK0, 3 << 14);
        gpio_delay();
        PUT32(GPIO_PUDCLK0, 0);

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
