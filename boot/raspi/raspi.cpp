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

#include <stdio.h>


extern "C" void BlinkLed();

char data[100];
char data2[] = { 1,2,3,4,5,6,7,8,9,10 };


// I get 0x410fd034 on my Raspberry Pi 3 (ARMv8 Cortex-A53)
//
// 41xxxxxx - Implementor (41 = ARM)
// xx0xxxxx - Variant (Major revision number)
// xxxFxxxx - Architecture Format Description (F = ARMv7)
// xxxxD03x - Part number
// xxxxxxx4 - Revision number

// Part number:
//  ARM1176     : 0x410fb767    0x410FB767 on Raspberry Pi 1
//  Cortex-A7   : 0x410fc070    0x410FC075 on Raspberry Pi 2
//  Cortex-A53  : 0x410fd034

#define ARM_CPUID_ARM1176   0x410fb760
#define ARM_CPUID_CORTEXA7  0x410fc070
#define ARM_CPUID_CORTEXA53 0x410fd030


unsigned cpu_id()
{
    unsigned value;
    asm("mrc p15,0,%0,c0,c0,0" : "=r"(value));
    return value;
}



extern "C" void raspi_main(unsigned bootDeviceId, unsigned machineId, const void* atags)
{
    int local;

    const unsigned cpuid = cpu_id() & ~0xF;   // Discard last 4 bits (revision number)
    const unsigned peripheral_base = (cpuid == ARM_CPUID_ARM1176) ? 0x20000000 : 0x3F000000;

    printf("Hello World from Raspberry Pi!\n");

    printf("bootDeviceId    : 0x%08x\n", bootDeviceId);
    printf("machineId       : 0x%08x\n", machineId);
    printf("atags at        : %p\n", atags);
    printf("cpu_id          : 0x%08x\n", cpu_id());
    printf("peripheral_base : 0x%08x\n", peripheral_base);
    printf("bss data at     : %p\n", data);
    printf("data2 at        : %p\n", data2);
    printf("stack around    : %p\n", &local);

    BlinkLed();
}
