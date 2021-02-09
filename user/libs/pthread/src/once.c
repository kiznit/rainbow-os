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
#include <rainbow/rainbow.h>


// https://www.remlab.net/op/futex-misc.shtml
int pthread_once(pthread_once_t* once, void (*init_routine)(void))
{
    // Already initialized (2) state - fast path
    if (atomic_load_explicit(&once->value, memory_order_acquire) == 2)
    {
        return 0;
    }

    // Move from uninitialized (0) to pending (1) state
    int value = 0;
    if (atomic_compare_exchange_strong_explicit(&once->value, &value, 1, memory_order_acq_rel, memory_order_acquire))
    {
        init_routine();

        // Move from pending (1) to initialized (2) state
        atomic_store_explicit(&once->value, 2, memory_order_release);

        // Wake up all blocked threads
        futex_broadcast((int*)&once->value);

        return 0;
    }

    // Wait for initialized (2) state - slow path
    while (value == 1)
    {
        futex_wait((int*)&once->value, 1);
        value = atomic_load_explicit(&once->value, memory_order_acquire);
    }

    return 0;
}
