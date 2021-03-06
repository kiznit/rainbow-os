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

#ifndef _RAINBOW_ARCH_ARM_SYSCALL_H
#define _RAINBOW_ARCH_ARM_SYSCALL_H

#ifdef __cplusplus
extern "C" {
#endif


static inline intptr_t __syscall0(intptr_t function)
{
    (void)function;
    return 0;
}


static inline intptr_t __syscall1(intptr_t function, intptr_t arg1)
{
    (void)function;
    (void)arg1;
    return 0;
}


static inline intptr_t __syscall2(intptr_t function, intptr_t arg1, intptr_t arg2)
{
    (void)function;
    (void)arg1;
    (void)arg2;
    return 0;
}


static inline intptr_t __syscall3(intptr_t function, intptr_t arg1, intptr_t arg2, intptr_t arg3)
{
    (void)function;
    (void)arg1;
    (void)arg2;
    (void)arg3;
    return 0;
}


static inline intptr_t __syscall4(intptr_t function, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4)
{
    (void)function;
    (void)arg1;
    (void)arg2;
    (void)arg3;
    (void)arg4;
    return 0;
}


static inline intptr_t __syscall5(intptr_t function, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5)
{
    (void)function;
    (void)arg1;
    (void)arg2;
    (void)arg3;
    (void)arg4;
    (void)arg5;
    return 0;
}


static inline intptr_t __syscall6(intptr_t function, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6)
{
    (void)function;
    (void)arg1;
    (void)arg2;
    (void)arg3;
    (void)arg4;
    (void)arg5;
    (void)arg6;
    return 0;
}


#ifdef __cplusplus
}
#endif

#endif
