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

#include "lock.h"
#include <rainbow/ipc.h>


void mutex_init(mutex_t* mutex)
{
    mutex->lock = 0;
}


void mutex_lock(mutex_t* mutex)
{
    // This check will lock the bus
    while (__sync_lock_test_and_set(&mutex->lock, 1))
    {
        syscall0(SYSCALL_YIELD);
    }
}


void mutex_unlock(mutex_t* mutex)
{
    __sync_lock_release(&mutex->lock);
}



void semaphore_init(semaphore_t* semaphore, unsigned int initialCount, unsigned int maxCount)
{
    mutex_init(&semaphore->mutex);
    semaphore->count = initialCount;
    semaphore->maxCount = maxCount;
}


void semaphore_signal(semaphore_t* semaphore)
{
    mutex_lock(&semaphore->mutex);

    if (semaphore->count < semaphore->maxCount)
    {
        ++semaphore->count;
    }

    mutex_unlock(&semaphore->mutex);
}


void semaphore_wait(semaphore_t* semaphore)
{
    for (;;)
    {
        mutex_lock(&semaphore->mutex);

        if (semaphore->count > 0)
        {
            // Lock acquired
            --semaphore->count;
            mutex_unlock(&semaphore->mutex);
            return;
        }

        // Yield
        mutex_unlock(&semaphore->mutex);
        syscall0(SYSCALL_YIELD);
    }
}
