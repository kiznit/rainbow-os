/*
    Copyright (c) 2016, Thierry Tremblay
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

#.include "kernel/x86.inc"

.equ X86_CR0_PE , (1 << 0)
.equ X86_CR0_MP , (1 << 1)
.equ X86_CR0_EM , (1 << 2)
.equ X86_CR0_TS , (1 << 3)
.equ X86_CR0_ET , (1 << 4)
.equ X86_CR0_NE , (1 << 5)
.equ X86_CR0_WP , (1 << 16)
.equ X86_CR0_AM , (1 << 18)
.equ X86_CR0_NW , (1 << 29)
.equ X86_CR0_CD , (1 << 30)
.equ X86_CR0_PG , (1 << 31)


.equ X86_CR4_OSFXSR      , (1 << 9)
.equ X86_CR4_OSXMMEXCPT  , (1 << 10)




/*
    Kernel entry point
*/

.section .text
.code32

.globl _start

_start:

    cli                         // Disable interrupts
    cld                         // Clear direction flag

    pushl %eax

    // Initialize FPU
    movl %cr0, %eax
    orl $X86_CR0_MP | X86_CR0_NE, %eax
    andl $~(X86_CR0_EM | X86_CR0_TS), %eax
    movl %eax, %cr0
    fninit

    // Initialize SSE
    movl %cr4, %eax
    orl $X86_CR4_OSFXSR | X86_CR4_OSXMMEXCPT, %eax
    movl %eax, %cr4

    popl %eax

    jmp kernel_main
