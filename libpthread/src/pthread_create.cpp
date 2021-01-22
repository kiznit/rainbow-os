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

#include <pthread.h>
#include <cerrno>
#include <sys/mman.h>
#include <rainbow.h>


// TODO: properly implement
extern "C" int pthread_create(pthread_t* thread, const pthread_attr_t* attr, void* (*start_routine)(void*), void* arg)
{
    (void)attr; // TODO: do something with attr

    const auto STACK_SIZE = 65536;

    const auto stack = mmap(nullptr, STACK_SIZE, PROT_WRITE, MAP_ANONYMOUS, -1, 0);
    if (stack == MAP_FAILED)
    {
        return EAGAIN;
    }

    //const int result = spawn_thread(start_routine, arg, 0, (char*)stack + STACK_SIZE, STACK_SIZE);
    const int result = syscall5(SYSCALL_THREAD, (intptr_t)start_routine, (intptr_t)arg, 0, (intptr_t)((char*)stack + STACK_SIZE), STACK_SIZE);
    if (result < 0)
    {
        munmap(stack, STACK_SIZE);
        return EAGAIN; // TODO: this is an assumption, need kernel error codes
    }

    // TODO: we need a thread if from the kernel
    *thread = (pthread_t)(uintptr_t)stack;

    return 0;
}
