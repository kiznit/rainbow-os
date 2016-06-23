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

.section .boot

.globl _start

    // The bootloader passes 3 arguments:
    //  r0 = 0
    //  r1 = 0xC42 (ID for Raspberry Pi)
    //  r2 = ATAGS

_start:
    // Initialize stack
    ldr sp, =_boot_stack_top

    // Clear BSS
    ldr r3, =_bss_start
    ldr r4, =_bss_end
    mov r5, #0
    mov r6, #0
    mov r7, #0
    mov r8, #0
    b .test
.loop:
    // 16 bytes at once
    stmia r3!, {r5-r8}
.test:
    cmp r3, r4
    blo .loop

    // Allow access to FPU in both Secure and Non-secure state
    mrc p15, 0, r3, c1, c1, 2
    orr r3, r3, #3 << 10
    mcr p15, 0, r3, c1, c1, 2

    // Initialize FPU
    ldr r3, =(0xF << 20)
    mcr p15, 0, r3, c1, c0, 2
    mov r3, #0x40000000
    vmsr FPEXC, r3

    // Jump to kernel_main
    ldr r3, =kernel_main
    blx r3

.halt:
    // todo: disable interrupts...?
    wfi
    b .halt



/*
    Boot Stack
*/

.section .bss
.balign 4096

_boot_stack:
.skip 65536
_boot_stack_top:
