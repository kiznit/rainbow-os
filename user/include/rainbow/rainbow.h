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

#ifndef _RAINBOW_H
#define _RAINBOW_H

#include <limits.h>
#include <stddef.h>
#include <rainbow/ipc.h>
#include <rainbow/syscall.h>


#ifdef __cplusplus
extern "C" {
#endif


// Block if (*futex == value), otherwise returns right away.
static inline int futex_wait(volatile int* futex, int value)
{
    return syscall2(SYSCALL_FUTEX_WAIT, (uintptr_t)futex, value);
}


// Wake up 'count' threads blocked on the futex.
// Returns the number of unblocked threads.
static inline int futex_wake(volatile int* futex, int count)
{
    return syscall2(SYSCALL_FUTEX_WAKE, (uintptr_t)futex, count);
}


// Wake up all threads blocked on the futex.
// Returns the number of unblocked threads.
static inline int futex_broadcast(volatile int* futex)
{
    return futex_wake(futex, INT_MAX);
}


#ifdef __cplusplus
}
#endif

#endif
