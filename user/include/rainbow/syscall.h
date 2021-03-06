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

#ifndef _RAINBOW_SYSCALL_H
#define _RAINBOW_SYSCALL_H

#include <stdint.h>

#if defined(__i386__)
#include <rainbow/arch/ia32/syscall.h>
#elif defined(__x86_64__)
#include <rainbow/arch/x86_64/syscall.h>
#elif defined(__arm__)
#include <rainbow/arch/arm/syscall.h>
#elif defined(__aarch64__)
#include <rainbow/arch/aarch64/syscall.h>
#endif

#define SYSCALL_EXIT            0
#define SYSCALL_MMAP            1
#define SYSCALL_MUNMAP          2
#define SYSCALL_THREAD          3
#define SYSCALL_IPC             4
#define SYSCALL_LOG             5   // Temporary until logger does it's job
#define SYSCALL_YIELD           6
#define SYSCALL_INIT_USER_TCB   7

#define SYSCALL_FUTEX_WAIT      8
#define SYSCALL_FUTEX_WAKE      9

#endif
