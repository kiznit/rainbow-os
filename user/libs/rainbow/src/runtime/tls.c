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

#include <stdlib.h>
#include <string.h>
#include <elf.h>
#include <sys/mman.h>
#include <rainbow/syscall.h>
#include <rainbow/usertask.h>

#if defined(__i386__)
typedef Elf32_Phdr Phdr;
#elif defined(__x86_64__)
typedef Elf64_Phdr Phdr;
#endif

extern long __aux[AT_COUNT];


static void* _tls_image;    // TLS binary image to copy
static int   _tls_length;   // TLS image length
static int   _tls_size;     // TLS total size (>= _tlslength)
static int   _tls_align;    // TLS alignment


UserTask* _alloc_thread()
{
    // TODO: take into account _tls_align (perhaps not necessary for main thread since align is 4096 by default)

    // We want to allocate TLS space + UserTask space.
    const int totalSize = _tls_size + sizeof(UserTask);

    // TODO: here we should not rely on mmap() but instead make a direct system call.
    //       At time of writing, this is not possible as user space determines the address.
    void* tls = mmap(0, totalSize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (tls == MAP_FAILED)
    {
        return 0;
    }

    memcpy(tls, _tls_image, _tls_size);

    // Calculate location of user space thread control block
    UserTask* thread = (UserTask*)(tls + _tls_size);

    return thread;
}


void _init_tls()
{
    const Phdr* phdr = (Phdr*)__aux[AT_PHDR];
    for (long i = 0; i != __aux[AT_PHNUM]; ++i, phdr = (Phdr*)((uintptr_t)phdr + __aux[AT_PHENT]))
    {
        if (phdr->p_type == PT_TLS)
        {
            _tls_image = (void*)(uintptr_t)(phdr->p_vaddr);
            _tls_length = phdr->p_filesz;
            _tls_size = phdr->p_memsz;
            _tls_align = phdr->p_align;
        }
    }

    // TODO: we could allocate a static buffer for the main thread's TLS and save allocating memory
    //       for app that don't need (or use very little) TLS.

    // Calculate location of user space thread control block
    UserTask* thread = _alloc_thread();

    // Initialize thread
    if (!thread || syscall1(SYSCALL_INIT_USER_TCB, (long)thread) < 0)
    {
        exit(-1);
    }
}
