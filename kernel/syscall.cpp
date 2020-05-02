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

#include <rainbow/syscall.h>
#include <kernel/kernel.hpp>
#include <kernel/usermode.hpp>


int SysCallInterrupt(InterruptContext* context)
{
    auto syscall = (SysCallParams*)context;

    switch (syscall->function)
    {
    case SYSCALL_EXIT:
        {
            // TODO
            for (;;);
        }
        break;

    case SYSCALL_LOG:
        {
            // TODO: pointer validation (don't want to crash or print kernel space memory!)
            const char* text = (char*)syscall->arg1;
            Log(text);
            syscall->result = 0; // Return success
        }
        break;

    case SYSCALL_MMAP:
        {
            // TODO: parameter validation, handling flags, etc
            const auto address = syscall->arg1;
            const auto length = syscall->arg2;
            const auto pageCount = align_up(length, MEMORY_PAGE_SIZE) >> MEMORY_PAGE_SHIFT;

            // TODO: provide an API to allocate 'x' continuous frames
            const void* virtualAddress = (void*)address;
            for (uintptr_t i = 0; i != pageCount; ++i)
            {
                auto frame = g_pmm->AllocatePages(1);
                g_vmm->m_pageTable->MapPages(frame, virtualAddress, 1, PAGE_PRESENT | PAGE_USER | PAGE_WRITE | PAGE_NX);
                virtualAddress = advance_pointer(virtualAddress, MEMORY_PAGE_SIZE);
            }

            syscall->result = address;
        }
        break;

    case SYSCALL_MUNMAP:
        {
            // TODO: parameter validation, handling flags, etc
            //const auto address = syscall->arg1;
            //const auto length = syscall->arg2;
            //const auto pageCount = align_up(length, MEMORY_PAGE_SIZE) >> MEMORY_PAGE_SHIFT;
            // TODO: g_vmm->FreePages(pageCount);
            syscall->result = 0;
        }
        break;

    case SYSCALL_THREAD:
        {
            // TODO: parameter validation, handling flags, etc
            const auto userFunction = (void*)syscall->arg1;
            const auto userArgs = (void*)syscall->arg2;
            const auto userFlags = (int)syscall->arg3;
            const auto userStack = (void*)syscall->arg4;
            const auto userStackSize = syscall->arg5;

            // Log("SYSCALL_THREAD:\n");
            // Log("    userFunction: %p\n", userFunction);
            // Log("    userArgs    : %p\n", userArgs);
            // Log("    userFlags   : %p\n", userFlags);
            // Log("    userStack   : %p\n", userStack);
            syscall->result = usermode_clone(userFunction, userArgs, userFlags, userStack, userStackSize);
        }
        break;

    default:
        {
            // Unknown function code, return error
            syscall->result = -1;
        }
    }

    return 1;
}
