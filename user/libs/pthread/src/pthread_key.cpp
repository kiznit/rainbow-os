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

// TODO: newlib doesn't provide PTHREAD_KEYS_MAX
#ifndef PTHREAD_KEYS_MAX
#define PTHREAD_KEYS_MAX 512
#endif


extern "C" int pthread_key_create(pthread_key_t* key, void (*destructor)(void*))
{
    // TODO: implement
    (void)key;
    (void)destructor;

    return 0;
}


/*
extern "C" int pthread_key_delete(pthread_key_t key)
{
    // TODO: implement
    (void)key;

    return 0;
}


extern "C" void* pthread_getspecific(pthread_key_t key)
{
    // TODO: implement
    (void)key;

    return 0;
}


extern "C" int pthread_setspecific(pthread_key_t key, const void* value)
{
    // TODO: implement
    (void)key;
    (void)value;

    return 0;
}
*/
