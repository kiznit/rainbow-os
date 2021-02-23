/*
    Copyright (c) 2021, Thierry Tremblay
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
#include <assert.h>
#include <errno.h>
#include <sys/mman.h>
#include <rainbow/rainbow.h>


pthread_t __alloc_thread();
void __init_newlib_thread();
void __pthread_run_destructors();


typedef struct
{
    pthread_t thread;
    void* (*userFunction)(void*);
    void* userArg;
} ThreadArgs;


static int thread_entry(const ThreadArgs* args)
{
    if (__syscall1(SYSCALL_INIT_USER_TCB, (long)args->thread) < 0)
    {
        // TODO: what do we want to do here? Can this even happen?
    }

    // Initialize the C runtime for this new thread
    __init_newlib_thread();

    void* retval = args->userFunction(args->userArg);

    pthread_exit(retval);

    return 0;
}


pthread_mutex_t __thread_list_lock = PTHREAD_MUTEX_INITIALIZER;


/*
    TODO: implementation incomplete, untested and bugged
*/


// TODO: implement properly
int pthread_cancel(pthread_t thread)
{
    // libgcc (and libstdc++) decides whether or not to use locks based on the existence of "pthread_cancel()".
    // If we don't provide this function, libgcc will think this program is not multithreaded and avoid using mutexes.
    // Because we are currently statically linking libpthread, make sure to include the "--whole-archive".

    (void)thread;
    assert(0);
    return ENOSYS;
}


int pthread_create(pthread_t* pThread, const pthread_attr_t* attr, void* (*userFunction)(void*), void* userArg)
{
    (void)attr; // TODO: do something with attr

    const int STACK_SIZE = 65536;

    void* stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (stack == MAP_FAILED)
    {
        return EAGAIN;
    }

    // TODO: we might want to allocate TLS + _pthread on the user stack when there is enough room
    pthread_t newThread = __alloc_thread();
    if (!newThread)
    {
        munmap(stack, STACK_SIZE);
        return EAGAIN;
    }

    // Build ThreadArgs on the new thread's stack.
    // TODO: is this how we want this to work? is it always safe? (probably not)
    ThreadArgs* threadArgs = (ThreadArgs*)stack;
    threadArgs->thread = newThread;
    threadArgs->userFunction = userFunction;
    threadArgs->userArg = userArg;

    pthread_mutex_lock(&__thread_list_lock);

    const int result = __syscall5(SYSCALL_THREAD, (long)thread_entry, (long)threadArgs, 0, (long)((char*)stack + STACK_SIZE), STACK_SIZE);

    if (result >= 0)
    {
        pthread_t self = pthread_self();
        newThread->next = self->next;
        newThread->prev = self;
        newThread->next->prev = newThread;
        newThread->prev->next = newThread;
    }

    pthread_mutex_unlock(&__thread_list_lock);

    if (result < 0)
    {
        munmap(stack, STACK_SIZE);
        return EAGAIN;
    }

    *pThread = newThread;

    return 0;
}


int pthread_detach(pthread_t thread)
{
    (void)thread;
    assert(0);
    return ENOSYS;
}


int pthread_join(pthread_t thread, void** retval)
{
    (void)thread;
    (void)retval;
    assert(0);
    return ENOSYS;
}


pthread_t pthread_self(void)
{
    pthread_t thread;
#if defined(__i386__)
    asm volatile ("movl %%gs:0x0, %0" : "=r"(thread));
#elif defined(__x86_64__)
    asm volatile ("movq %%fs:0x0, %0" : "=r"(thread));
#else
#error Not implemented
#endif
    return thread;
}


int pthread_equal(pthread_t t1, pthread_t t2)
{
    return t1->id == t2->id;
}


void pthread_exit(void* retval)
{
    (void)retval;

    __pthread_run_destructors();

    pthread_t self = pthread_self();

    pthread_mutex_lock(&__thread_list_lock);
    self->next->prev = self->prev;
    self->prev->next = self->next;
    pthread_mutex_unlock(&__thread_list_lock);

    // TODO: release memory for the current thread (self)
    // TODO: wake any joiner
    // TODO: exit!
    for (;;);
}
