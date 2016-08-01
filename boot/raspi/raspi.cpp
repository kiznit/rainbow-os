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

#define ATAG_NONE       0x00000000
#define ATAG_CORE       0x54410001
#define ATAG_MEM        0x54410002
#define ATAG_VIDEOTEXT  0x54410003
#define ATAG_RAMDISK    0x54410004
#define ATAG_INITRD2    0x54420005
#define ATAG_SERIAL     0x54410006
#define ATAG_REVISION   0x54410007
#define ATAG_VIDEOLFB   0x54410008



char data[100];
char data2[] = { 1,2,3,4,5,6,7,8,9,10 };

extern "C" void raspi_main(unsigned r0, unsigned id, const void* atags)
{
    // I am getting atags = NULL on my Raspberry Pi 3, but there are atags at 0x100!
    // This is the workaround, and it is probably safe on any Raspberry Pi.
    if (atags == NULL)
    {
        const unsigned* check = (const unsigned*)0x100;
        if (check[1] == ATAG_CORE && (check[0] == 2 || check[0] == 5))
        {
            atags = check;
        }
    }

    int local;

    printf("Hello World from Raspberry Pi 3!\n");

    printf("r0          : 0x%08x\n", r0);
    printf("id          : 0x%08x\n", id);
    printf("atags at    : %p\n", atags);
    printf("bss data at : %p\n", data);
    printf("data2 at    : %p\n", data2);
    printf("stack around: %p\n", &local);

    BlinkLed();
}
