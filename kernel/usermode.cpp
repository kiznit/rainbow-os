/*
    Copyright (c) 2020, Thierry Tremblay
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

#include "usermode.hpp"
#include <kernel/kernel.hpp>
#include "elf.hpp"


typedef void (*UserSpaceEntryPoint)();
extern "C" void JumpToUserMode(UserSpaceEntryPoint entryPoint, void* userStack);

extern int SysCallInterrupt(InterruptContext*);



void usermode_init()
{
    interrupt_register(0x80, SysCallInterrupt);
}



static void usermode_entry(void* args)
{
    const Module* module = (Module*)args;

    Log("User module at %X, size is %X\n", module->address, module->size);

    const physaddr_t entryPoint = elf_map(module->address, module->size);
    if (!entryPoint)
    {
        Fatal("Could not load / start user process\n");
    }

    Log("Module entry point at %X\n", entryPoint);

// TODO: use constants for these, do not check for arch!
#if defined(__i386__)
    void* stack = (void*)0xE0000000; // TODO: should be 0xF0000000
#elif defined(__x86_64__)
    void* stack = (void*)0x0000800000000000;
#endif

    JumpToUserMode((UserSpaceEntryPoint)entryPoint, stack);
}



void usermode_spawn(const Module* module)
{
    Task::Create(usermode_entry, module, 0);
}
