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

/*
    This file provides a "fake" implementation of POSIX threads.
    This is required by some runtime components (libgcc, libstdc++).

    The future might be to add our own threading model to libgcc instead of
    relying on this fake POSIX implementation.
*/

#include <cerrno>
#include <pthread.h>

/*
// libgcc decides whether or not to use locks based on the existence of "pthread_cancel".
// So if we don't provide this function, libgcc will think this is not a multithreaded
// environment (but it is!).
extern "C" int pthread_cancel(pthread_t thread)
{
    (void)thread;
    return 0;
}


// Once you provide pthread_cancel, libgcc will start using POSIX mutexes, so there need
// to be some implementation... To find which functions are required, look at the disassembly
// and do a search for "call   0 ": this will show you all the locations where pthread functions
// are used and missing. To find which functions they are, one will need to guess or look at
// libgcc's source code.
extern "C" int pthread_mutex_lock(pthread_mutex_t* mutex)
{
    *mutex = 1;
    return 0;
}


extern "C" int pthread_mutex_trylock(pthread_mutex_t* mutex)
{
    if (*mutex)
    {
        return EBUSY;
    }

    *mutex = 1;
    return 0;
}


extern "C" int pthread_mutex_unlock(pthread_mutex_t* mutex)
{
    *mutex = 0;
    return 0;
}
*/
