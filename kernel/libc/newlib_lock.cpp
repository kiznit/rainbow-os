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

#include <sys/lock.h>


// TODO: this following are just stubs at this point, we need to implement the locks!

struct __lock
{
    char unused;
};


struct __lock __lock___sinit_recursive_mutex;
struct __lock __lock___sfp_recursive_mutex;
struct __lock __lock___atexit_recursive_mutex;
struct __lock __lock___at_quick_exit_mutex;
struct __lock __lock___malloc_recursive_mutex;
struct __lock __lock___env_recursive_mutex;
struct __lock __lock___tz_mutex;
struct __lock __lock___dd_hash_mutex;
struct __lock __lock___arc4random_mutex;


extern "C" void __retarget_lock_init(_LOCK_T* lock)
{
   (void)lock;
}


extern "C" void __retarget_lock_init_recursive(_LOCK_T* lock)
{
   (void)lock;
}


extern "C" void __retarget_lock_close(_LOCK_T lock)
{
   (void)lock;
}


extern "C" void __retarget_lock_close_recursive(_LOCK_T lock)
{
   (void)lock;
}


extern "C" void __retarget_lock_acquire(_LOCK_T lock)
{
   (void)lock;
}


extern "C" void __retarget_lock_acquire_recursive(_LOCK_T lock)
{
   (void)lock;
}


extern "C" int __retarget_lock_try_acquire(_LOCK_T lock)
{
   (void)lock;
    return 1;
}


extern "C" int __retarget_lock_try_acquire_recursive(_LOCK_T lock)
{
   (void)lock;
    return 1;
}


extern "C" void __retarget_lock_release(_LOCK_T lock)
{
   (void)lock;
}


extern "C" void __retarget_lock_release_recursive(_LOCK_T lock)
{
   (void)lock;
}
