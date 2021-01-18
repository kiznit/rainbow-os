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

#include <rainbow.h>
#include <stdlib.h>


void _init_newlib_thread();


typedef struct ThreadArgs
{
    int (*userFunction)(void*);
    const void* userArgs;
} ThreadArgs;



static int s_thread_entry(ThreadArgs* p)
{
    _init_newlib_thread();

    int result = p->userFunction((void*)p->userArgs);

    free(p);

    return result;
}


int spawn_thread(int (*userFunction)(void*), const void* userArgs, int flags, void* stack, size_t stackSize)
{
    ThreadArgs* p = malloc(sizeof(ThreadArgs));
    p->userFunction = userFunction;
    p->userArgs = userArgs;

    const int result = syscall5(SYSCALL_THREAD, (intptr_t)s_thread_entry, (intptr_t)p, flags, (intptr_t)stack, stackSize);

    if (result < 0)
    {
        free(p);
    }

    return result;
}
