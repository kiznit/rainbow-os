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

.section .text

.globl _start
_start:
    // Initialize stack
    mov sp, #0x8000

    // Clear out BSS
    ldr r4, =_bss_start
    ldr r5, =_bss_end
    mov r6, #0
    mov r7, #0
    mov r8, #0
    mov r9, #0
    b .test
.loop:
    // 16 bytes at once
    stmia r4!, {r6-r9}
.test:
    cmp r4, r5
    blo .loop

    // Allow access to FPU in both Secure and Non-secure state
    mrc p15, 0, r0, c1, c1, 2
    orr r0, r0, #3 << 10
    mcr p15, 0, r0, c1, c1, 2

    // Initialize FPU
    ldr r0, =(0xF << 20)
    mcr p15, 0, r0, c1, c0, 2
    mov r3, #0x40000000
    vmsr FPEXC, r3

    b kernel_main


.globl dummy
dummy:
    bx lr
