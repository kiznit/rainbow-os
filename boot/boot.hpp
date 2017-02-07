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

#ifndef _RAINBOW_BOOT_HPP
#define _RAINBOW_BOOT_HPP

#include <stddef.h>
#include <stdint.h>


//todo: this needs to be defined by arch?
typedef uint64_t physaddr_t;


#define ARRAY_LENGTH(array) (sizeof(array) / sizeof((array)[0]))

#define STRINGIZE_DELAY(x) #x
#define STRINGIZE(x) STRINGIZE_DELAY(x)



struct BootInfo
{
    physaddr_t  initrdAddress;  // initrd physical address in memory
    physaddr_t  initrdSize;     // Size of initrd
};


extern BootInfo g_bootInfo;


// Prepare the OS for execution (this will load the kernel and put everything in the right place)
void boot_setup();

// Jump to the kernel
void boot_jump_to_kernel();


#endif
