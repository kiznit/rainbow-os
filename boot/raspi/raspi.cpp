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

#include <stdint.h>
#include <stdio.h>
#include <endian.h>

#include "boot.hpp"
#include "atags.hpp"
#include "fdt.hpp"



// Models are a combinaison of implementor and part number.
#define ARM_CPU_MODEL_ARM1176   0x4100b760
#define ARM_CPU_MODEL_CORTEXA7  0x4100c070
#define ARM_CPU_MODEL_CORTEXA53 0x4100d030
#define ARM_CPU_MODEL_MASK      0xff00fff0


inline unsigned arm_cpuid_id()
{
    // Retrieve the processor's Main ID Register (MIDR)
    unsigned value;
    asm("mrc p15,0,%0,c0,c0,0" : "=r"(value) : : "cc");
    return value;
}

inline unsigned arm_cpuid_model()
{
    return arm_cpuid_id() & ARM_CPU_MODEL_MASK;
}


static uint32_t PERIPHERAL_BASE;

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

#define read_barrier()  __asm__ __volatile__ ("" : : : "memory")
#define write_barrier() __asm__ __volatile__ ("" : : : "memory")


inline uint32_t mmio_read32(const volatile void* address)
{
    uint32_t value;
    asm volatile("ldr %1, %0" : "+Qo" (*(volatile uint32_t*)address), "=r" (value));
    read_barrier();
    return value;
}


inline void mmio_write32(volatile void* address, uint32_t value)
{
    write_barrier();
    asm volatile("str %1, %0" : "+Qo" (*(volatile uint32_t*)address) : "r" (value));
}


#define GET32(x) mmio_read32((void*)x)
#define PUT32(x,y) mmio_write32((void*)x, y)


class RaspberryUart
{
public:

    void Initialize()
    {
        unsigned int ra;

        // Disable UART 0
        PUT32(UART0_CR, 0);

        // Map UART0 (alt function 0) to GPIO pins 14 and 15
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

        // Clear pending interrupts
        PUT32(UART0_ICR, 0x7FF);

        // Baud rate
        // Divider = UART_CLOCK / (16 * Baud)
        // Fraction = (Fraction part * 64) + 0.5

        // Raspberry 2: UART CLOCK = 3000000 (3MHz)
        // Divider = 3000000 / (16 * 115200) = 1.627    --> 1
        // Fraction = (.627 * 64) + 0.5 = 40.6          --> 40
        //PUT32(UART0_IBRD, 1);
        //PUT32(UART0_FBRD, 40);

        // Raspberry 3: UART_CLOCK = 48000000 (48 MHz)
        // Divider = 48000000 / (16 * 115200) = 26.041766667  --> 26
        // Fraction = (.041766667 * 64) + 0.5 = 3.1666667     --> 3
        PUT32(UART0_IBRD, 26);
        PUT32(UART0_FBRD, 3);

        // Enable FIFO, 8-N-1
        PUT32(UART0_LCRH, 0x70);

        // Mask all interrupts
        PUT32(UART0_IMSC, 0x7F2);

        // Enable UART0 (receive + transmit)
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

        if (c == '\n')
        {
            putc('\r');
        }
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



static RaspberryUart uart;



extern "C" int _libc_print(const char* string)
{
    size_t length = 0;
    for (const char* p = string; *p; ++p, ++length)
    {
        uart.putc(*p);
    }

    return length;
}



static void ProcessAtags(const atag::Tag* atags)
{
    printf("ATAGS:\n");

    for (const atag::Tag* tag = atags; tag; tag = tag->next())
    {
        switch (tag->type)
        {
        case atag::ATAG_CORE:
            if (tag->size > 2)
            {
                // My RaspberryPi 3 says flags = 0, pageSize = 0, rootDevice = 0. Mmm.
                auto core = static_cast<const atag::Core*>(tag);
                printf("    ATAG_CORE   : flags = 0x%08lx, pageSize = 0x%08lx, rootDevice = 0x%08lx\n", core->flags, core->pageSize, core->rootDevice);
            }
            else
            {
                printf("    ATAG_CORE   : no data\n");
            }
            break;

        case atag::ATAG_MEMORY:
            {
                // My RaspberryPi 3 has one entry: address 0, size 0x3b000000
                auto memory = static_cast<const atag::Memory*>(tag);
                printf("    ATAG_MEMORY : address = 0x%08lx, size = 0x%08lx\n", memory->address, memory->size);
            }
            break;

        case atag::ATAG_INITRD2:
            {
                // Works fine (that's good)
                auto initrd = static_cast<const atag::Initrd2*>(tag);
                printf("    ATAG_INITRD2: address = 0x%08lx, size = 0x%08lx\n", initrd->address, initrd->size);
                g_bootInfo.initrdAddress = initrd->address;
                g_bootInfo.initrdSize = initrd->size;
            }
            break;

        case atag::ATAG_CMDLINE:
            {
                // My RaspberryPi 3 has a lot to say:
                // "dma.dmachans=0x7f35 bcm2708_fb.fbwidth=656 bcm2708_fb.fbheight=416 bcm2709.boardrev=0xa22082 bcm2709.serial=0xe6aaac09 smsc95xx.macaddr=B8:27:EB:AA:AC:09
                //  bcm2708_fb.fbswap=1 bcm2709.uart_clock=48000000 vc_mem.mem_base=0x3dc00000 vc_mem.mem_size=0x3f000000  console=ttyS0,115200 kgdboc=ttyS0,115200 console=tty1
                //  root=/dev/mmcblk0p2 rootfstype=ext4 rootwait"
                auto commandLine = static_cast<const atag::CommandLine*>(tag);
                printf("    ATAG_CMDLINE: \"%s\"\n", commandLine->commandLine);
            }
            break;

        default:
            printf("    Unhandled ATAG: 0x%08lx\n", tag->type);
        }
    }
}



// ref:
//  https://chromium.googlesource.com/chromiumos/third_party/dtc/+/master/fdtdump.c

static void ProcessDeviceTree(const fdt_header* deviceTree)
{
    printf("Device tree:\n");
    printf("    totalsize           : %08lx\n", betoh32(deviceTree->totalsize));
    printf("    off_dt_struct       : %08lx\n", betoh32(deviceTree->off_dt_struct));
    printf("    off_dt_strings      : %08lx\n", betoh32(deviceTree->off_dt_strings));
    printf("    off_mem_rsvmap      : %08lx\n", betoh32(deviceTree->off_mem_rsvmap));
    printf("    version             : %08lx\n", betoh32(deviceTree->version));
    printf("    last_comp_version   : %08lx\n", betoh32(deviceTree->last_comp_version));
    printf("    boot_cpuid_phys     : %08lx\n", betoh32(deviceTree->boot_cpuid_phys));

    const fdt_reserve_entry* rsvmap = (fdt_reserve_entry*)(uintptr_t(deviceTree) + betoh32(deviceTree->off_mem_rsvmap));
    printf("\nReserved memory map (%p):\n", rsvmap);;

    for ( ; rsvmap->size != 0; ++rsvmap)
    {
        uint64_t address = betoh64(rsvmap->address);
        uint64_t size = betoh64(rsvmap->size);
        printf("    %016llx: %016llx bytes\n", address, size);
    }

    //todo: make sure to add the device tree itself to the reserved memory map (it should be but isn't)


    // const fdt_node_header* nodes = (fdt_node_header*)(uintptr_t(deviceTree) + betoh32(deviceTree->off_dt_struct));
    // printf("\nodes (%p):\n", nodes);

    // int depth = 0;

    // for (const fdt_node_header* node = nodes; node->tag != FDT_END; )
    // {
    //     switch (node->tag)
    //     {
    //     case FDT_BEGIN_NODE:
    //         {
    //             ++depth;
    //         }
    //         break;

    //     case FDT_END_NODE:
    //         {
    //         }
    //         break;

    //     case FDT_PROP:
    //         {
    //         }
    //         break;

    //     case FDT_NOP:
    //         {
    //         }
    //         break;

    //     case FDT_END:
    //         {
    //             --depth;
    //             node
    //         }
    //         break;
    //     }

    //     // Move to next node
    // }
}




/*
    Check this out for detecting Raspberry Pi Model:

        https://github.com/mrvn/RaspberryPi-baremetal/tree/master/004-a-t-a-and-g-walk-into-a-baremetal

    Peripheral base address detection:

        https://www.raspberrypi.org/forums/viewtopic.php?t=127662&p=854371
*/

extern "C" void raspi_main(unsigned bootDeviceId, unsigned machineId, const void* params)
{
    // My Raspberry Pi 3 doesn't pass in the atags address in 'params', but they sure are there at 0x100
    if (params == nullptr)
    {
        // Check if atags are available in the usual location (0x100)
        const atag::Tag* atags = reinterpret_cast<const atag::Tag*>(0x100);
        if (atags->type == atag::ATAG_CORE)
        {
            params = atags;
        }
    }

    // Peripheral base address
    PERIPHERAL_BASE = arm_cpuid_model() == ARM_CPU_MODEL_ARM1176 ? 0x20000000 : 0x3F000000;

    uart.Initialize();

    // Clear screen and set cursor to (0,0)
    printf("\033[m\033[2J\033[;H");

    // Rainbow
    printf("\033[31mR\033[1ma\033[33mi\033[1;32mn\033[36mb\033[34mo\033[35mw\033[m");

    printf(" Raspberry Pi Bootloader\n\n");

    printf("bootDeviceId    : 0x%08x\n", bootDeviceId);
    printf("machineId       : 0x%08x\n", machineId);
    printf("params          : %p\n", params);
    printf("cpu_id          : 0x%08x\n", arm_cpuid_id());
    printf("peripheral_base : 0x%08lx\n", PERIPHERAL_BASE);
    printf("\n");


    // Check for flattened device tree (FDT) first
    const fdt_header* deviceTree = reinterpret_cast<const fdt_header*>(params);
    const atag::Tag* atags = reinterpret_cast<const atag::Tag*>(0x100);

    if (deviceTree->magic == FDT_HEADER)
    {
        ProcessDeviceTree(deviceTree);
    }
    else if (atags->type == atag::ATAG_CORE)
    {
        ProcessAtags(atags);
    }
    else
    {
        printf("No boot parameters (atags or device tree) detected!\n");
    }

    if (g_bootInfo.initrdAddress && g_bootInfo.initrdSize)
    {
        boot_setup();
        boot_jump_to_kernel();
    }
}
