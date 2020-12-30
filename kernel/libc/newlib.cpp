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

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <reent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <metal/helpers.hpp>
#include <metal/log.hpp>


// TODO: what's the right upper bound here?
// TODO: if we want to support reentrancy at some point, we will need this per-cpu
// TODO: move to percpu data and/or TLS?
static _reent s_contexts[8];


void newlib_init()
{
    // Set newlib to use the first context
    _impure_ptr = s_contexts;

    // Initialize the context
    _REENT_INIT_PTR_ZEROED(_impure_ptr);

    //Log("newlib_init\n");
}


void newlib_push_context()
{
    // Allocate context
    assert((unsigned long)(_impure_ptr - s_contexts) < ARRAY_LENGTH(s_contexts));
    ++_impure_ptr;

    // Initialize the context (we don't know if it is zero-ed, so we need to first clear the context)
    memset(_impure_ptr, 0, sizeof(*_impure_ptr));
    _REENT_INIT_PTR_ZEROED(_impure_ptr);
}


void newlib_pop_context()
{
    // Free current context
    --_impure_ptr;
    assert(_impure_ptr >= s_contexts);
}



/*
    Newlib system calls

    These are called by newlib, which is C code and not C++ exception safe code.
    This means that all system calls need to be "noexcept". If they throw anything,
    the internal state of newlib could be corrupted.
*/

extern "C" int close(int fd) noexcept
{
    // TODO
    (void)fd;

    errno = ENOTSUP;
    return -1;
}


extern "C" void _exit(int status) noexcept
{
    // TODO
    Fatal("_exit() called with status %x\n", status);
}


extern "C" int fstat(int fd, struct stat* pstat) noexcept
{
    // TODO
    (void)fd;

    pstat->st_mode = S_IFCHR;
    errno = 0;
    return 0;
}


extern "C" int getpid() noexcept
{
    // TODO
    return 1;
}


extern "C" int isatty(int fd) noexcept
{
    // TODO
    (void)fd;

    errno = 0;
    return 1;
}


extern "C" int kill(int pid, int signal) noexcept
{
    // TODO
    (void)pid;
    (void)signal;

    errno = ENOTSUP;
    return -1;
}


extern "C" off_t lseek(int fd, off_t position, int whence) noexcept
{
    // TODO
    (void)fd;
    (void)position;
    (void)whence;

    errno = 0;
    return 0;
}


extern "C" _READ_WRITE_RETURN_TYPE read(int fd, void* buffer, size_t count) noexcept
{
    // TODO
    (void)fd;
    (void)buffer;
    (void)count;

    errno = ENOTSUP;
    return -1;
}


extern "C" _READ_WRITE_RETURN_TYPE write(int fd, const void* buffer, size_t count) noexcept
{
    // TODO
    (void)fd;

    // TODO: do we need to take "count" into account?
    console_print((const char*)buffer, count);

    errno = 0;
    return count;
}


extern "C" void* _malloc_r(_reent* reent, size_t size) noexcept
{
    reent->_errno = 0;
    return malloc(size);
}


extern "C" void _free_r(_reent* reent, void* p) noexcept
{
    reent->_errno = 0;
    free(p);
}


extern "C" void* _calloc_r(_reent* reent, size_t size, size_t length) noexcept
{
    reent->_errno = 0;
    return calloc(size, length);
}


extern "C" void* _realloc_r(_reent* reent, void* p, size_t size) noexcept
{
    reent->_errno = 0;
    return realloc(p, size);
}
