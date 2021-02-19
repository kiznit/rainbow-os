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

#include <sys/mman.h>
#include <errno.h>
#include <rainbow/syscall.h>

// TODO: these definitions do not belong here, they also need to be in sync with <kernel/config.hpp>.
#if defined(__i386__)
static void* const VMA_USER_END = (void*)0xF0000000;
#elif defined(__x86_64__)
static void* const VMA_USER_END = (void*)0x0000800000000000ull;
#endif


void* mmap(void* address, size_t length, int protection, int flags, int fd, off_t offset)
{
    // TODO
    (void)protection;
    (void)flags;

    if (address || length == 0 || fd != -1 || offset)
    {
        errno = EINVAL;
        return MAP_FAILED;
    }

    void* memory = (void*)__syscall2(SYSCALL_MMAP, (long)address, length);
    if (memory >= VMA_USER_END)
    {
        errno = -(long)memory;
        return MAP_FAILED;
    }

    return memory;
}
