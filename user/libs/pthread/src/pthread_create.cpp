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
#include <rainbow/rainbow.h>
#include <rainbow/usertask.h>

extern "C" UserTask* _alloc_thread();

struct ThreadArgs
{
    UserTask* thread;
    void* (*userFunction)(void*);
    void* userArg;
};


static int thread_entry(const ThreadArgs* args)
{
    if (syscall1(SYSCALL_INIT_USER_TCB, (long)args->thread) < 0)
    {
        // TODO: what do we want to do here? Can this even happen?
    }

    void* retval = args->userFunction(args->userArg);

    pthread_exit(retval);

    return 0;
}


// TODO: properly implement
extern "C" int pthread_create(pthread_t* pThread, const pthread_attr_t* attr, void* (*userFunction)(void*), void* userArg)
{
    (void)attr; // TODO: do something with attr

    const auto STACK_SIZE = 65536;

    const auto stack = mmap(nullptr, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (stack == MAP_FAILED)
    {
        return EAGAIN;
    }

    // TODO: we might want to allocate TLS + UserTask on the user stack when there is enough room
    UserTask* thread = _alloc_thread();
    if (!thread)
    {
        munmap(stack, STACK_SIZE);
        return EAGAIN;
    }

    // Build ThreadArgs on the new thread's stack.
    // TODO: is this how we want this to work? is it always safe? (probably not)
    ThreadArgs* threadArgs = (ThreadArgs*)stack;
    threadArgs->thread = thread;
    threadArgs->userFunction = userFunction;
    threadArgs->userArg = userArg;

    const int result = syscall5(SYSCALL_THREAD, (intptr_t)thread_entry, (intptr_t)threadArgs, 0, (intptr_t)((char*)stack + STACK_SIZE), STACK_SIZE);
    if (result < 0)
    {
        munmap(stack, STACK_SIZE);
        return EAGAIN;
    }

    *pThread = (void*)thread;

    return 0;
}



extern "C" void pthread_exit(void* retval)
{
    (void)retval;

    // TODO: cleanup, key destructors, etc...
}
