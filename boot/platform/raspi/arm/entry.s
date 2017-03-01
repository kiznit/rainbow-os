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

/******************************************************************************

    Program enty point


    Environment at boot

     The ATAG boot protocol defines a sane state for the system to be in before calling the kernel. Namely this is:

      - The CPU must be in SVC (supervisor) mode with both IRQ and FIQ interrupts disabled.
      - The MMU must be off, i.e. code running from physical RAM with no translated addressing.
      - Data cache must be off
      - Instruction cache may be either on or off
      - CPU register 0 must be 0
      - CPU register 1 must be the ARM Linux machine type
      - CPU register 2 must be the physical address of the parameter list


    The bootloader passes 3 arguments:
        r0 = 0     (Boot device ID)
        r1 = 0xC42 (ARM Linux Machine ID for Broadcom BCM2708 Video Coprocessor)
        r2 = ATAGS or Device Tree Blob (dtb)

    Preserve these registers! We want to pass them to raspi_main()

******************************************************************************/

.section .boot

      org = 0x8000

.globl _start
_start:

    // Initialize the stack
    ldr sp, =_boot_stack_top

    // Turn on unaligned memory access
    mrc p15, #0, r4, c1, c0, #0
    orr r4, #0x400000
    mcr p15, #0, r4, c1, c0, #0

    // Initialize FPU
    ldr r3, =(0xF << 20)
    mcr p15, #0, r3, c1, c0, #2
    mov r3, #0x40000000
    vmsr FPEXC, r3

    // Jump to raspi_main
    bl raspi_main

.halt:
    cpsid if    // TODO: is this the right way to disable interrupts?
    wfi
    b .halt



/******************************************************************************

    Helper to introduce CPU delay

******************************************************************************/

.globl cpu_delay
cpu_delay:
    bx lr



/******************************************************************************

    Boot Stack

******************************************************************************/

.section .bss
.align 12

_boot_stack:
.skip 32768
_boot_stack_top:
