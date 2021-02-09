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

#ifndef __SYS_PTHREADTYPES_H
#define __SYS_PTHREADTYPES_H

// One cannot simply #include <stdatomic.h> when compiling C++ code with GCC.
// We work around this as best we can.

#ifdef __cplusplus
typedef volatile int atomic_int;
#else
// It is necessary to #include <stddef.h> and <stdint.h> for this file to
// compile when building newlib.
#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


typedef struct _pthread* pthread_t;
typedef struct _pthread_cond pthread_cond_t;
typedef struct _pthread_mutex pthread_mutex_t;
typedef struct _pthread_once pthread_once_t;
typedef struct _pthread_rwlock pthread_rwlock_t;

// TODO:
typedef int pthread_attr_t;
typedef int pthread_condattr_t;
typedef int pthread_key_t;
typedef int pthread_mutexattr_t;
typedef int pthread_rwlockattr_t;


// TODO: do any these need to be public? musl puts these in <limits.h> which is... interesting
#define PTHREAD_DESTRUCTOR_ITERATIONS 10
#define PTHREAD_KEYS_MAX 512


struct _pthread
{
    // Part of the ABI
    pthread_t   self;       // Self-pointer
    int         id;         // Task id

    // Not part of the ABI
    pthread_t   next;       // Next thread in the process
    pthread_t   prev;       // Previous thread in the process

    // TODO: allocate this dynamically as needed/used?
    void*       keyValues[PTHREAD_KEYS_MAX]; // pthread key values
};


struct _pthread_cond
{
    atomic_int value;
    atomic_int sequence;
};


struct _pthread_mutex
{
    atomic_int type;
    atomic_int value;
    atomic_int owner;
    atomic_int count;
};


struct _pthread_once
{
    atomic_int value;
};


struct _pthread_rwlock
{
    atomic_int type;
    atomic_int value;
    atomic_int readers;
    atomic_int writers;
};


#ifdef __cplusplus
}
#endif

#endif
