/*
    Copyright (c) 2016, Thierry Tremblay
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

/*
#include <stdio.h>
#include <stdlib.h>
#include <arch/cpuid.hpp>
#include "raspi.hpp"
#include "emmc.hpp"
#include "mailbox.hpp"



extern "C" void BlinkLed();

char data[100];
char data2[] = { 1,2,3,4,5,6,7,8,9,10 };



static void DetectMachine(MachineDescription* machine)
{
    switch (arm_cpuid_model())
    {
    case ARM_CPU_MODEL_ARM1176:
        machine->model = Model_Raspberry;
        machine->peripheral_base = 0x20000000;
        break;

    case ARM_CPU_MODEL_CORTEXA7:
        machine->model = Model_Raspberry2;
        machine->peripheral_base = 0x3F000000;
        break;

    case ARM_CPU_MODEL_CORTEXA53:
        machine->model = Model_Raspberry3;
        machine->peripheral_base = 0x3F000000;
        break;

    default:
        abort();
    }
}
*/


/*
    Check this out for detecting Raspberry Pi Model:

        https://github.com/mrvn/RaspberryPi-baremetal/tree/master/004-a-t-a-and-g-walk-into-a-baremetal

    Peripheral base address detection:

        https://www.raspberrypi.org/forums/viewtopic.php?t=127662&p=854371
*/


extern "C" void raspi_main(unsigned bootDeviceId, unsigned machineId, const void* atags)
{
    (void)bootDeviceId;
    (void)machineId;
    (void)atags;
/*
    int local;

    MachineDescription machine;
    DetectMachine(&machine);

    // Clear screen and set cursor to (0,0)
    printf("\033[2J\033[;H");

    printf("Hello World from Raspberry Pi!\n\n");

    printf("bootDeviceId    : 0x%08x\n", bootDeviceId);
    printf("machineId       : 0x%08x\n", machineId);
    printf("atags at        : %p\n", atags);
    printf("cpu_id          : 0x%08x\n", arm_cpuid_id());
    printf("peripheral_base : 0x%08x\n", machine.peripheral_base);
    printf("bss data at     : %p\n", data);
    printf("data2 at        : %p\n", data2);
    printf("stack around    : %p\n", &local);

    printf("\nCalling mailbox interface...\n");

    Mailbox mailbox(machine);
    Mailbox::MemoryRange memory;

    if (mailbox.GetARMMemory(&memory) < 0)
        printf("*** Failed to read ARM memory\n");
    else
        printf("ARM memory      : 0x%08x - 0x%08x\n", (unsigned)memory.address, (unsigned)(memory.address + memory.size));

    if (mailbox.GetVCMemory(&memory) < 0)
        printf("*** Failed to read VC memory\n");
    else
        printf("VC memory       : 0x%08x - 0x%08x\n", (unsigned)memory.address, (unsigned)(memory.address + memory.size));

    BlinkLed();
*/
}
