# Copyright (c) 2022, Thierry Tremblay
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

.section .text

.equ ALL_REGS_SIZE, 16 * 16

.macro SAVE_ALL_REGS
    sub sp, sp, #ALL_REGS_SIZE
	stp	x0, x1, [sp, #16 * 0]
	stp	x2, x3, [sp, #16 * 1]
	stp	x4, x5, [sp, #16 * 2]
	stp	x6, x7, [sp, #16 * 3]
	stp	x8, x9, [sp, #16 * 4]
	stp	x10, x11, [sp, #16 * 5]
	stp	x12, x13, [sp, #16 * 6]
	stp	x14, x15, [sp, #16 * 7]
	stp	x16, x17, [sp, #16 * 8]
	stp	x18, x19, [sp, #16 * 9]
	stp	x20, x21, [sp, #16 * 10]
	stp	x22, x23, [sp, #16 * 11]
	stp	x24, x25, [sp, #16 * 12]
	stp	x26, x27, [sp, #16 * 13]
	stp	x28, fp, [sp, #16 * 14]
    add x9, sp, #ALL_REGS_SIZE
    stp	x30, x9, [sp, #16 * 15]
.endm

.macro RESTORE_ALL_REGS
	ldp	x0, x1, [sp, #16 * 0]
	ldp	x2, x3, [sp, #16 * 1]
	ldp	x4, x5, [sp, #16 * 2]
	ldp	x6, x7, [sp, #16 * 3]
	ldp	x8, x9, [sp, #16 * 4]
	ldp	x10, x11, [sp, #16 * 5]
	ldp	x12, x13, [sp, #16 * 6]
	ldp	x14, x15, [sp, #16 * 7]
	ldp	x16, x17, [sp, #16 * 8]
	ldp	x18, x19, [sp, #16 * 9]
	ldp	x20, x21, [sp, #16 * 10]
	ldp	x22, x23, [sp, #16 * 11]
	ldp	x24, x25, [sp, #16 * 12]
	ldp	x26, x27, [sp, #16 * 13]
	ldp	x28, x29, [sp, #16 * 14]
	ldp	x30, x31, [sp, #16 * 15]
    add sp, sp, #ALL_REGS_SIZE
.endm

// We have 0x80 bytes of instruction for each vector entry
.macro EXCEPTION_ENTRY handler
    .balign 0x80
    .InterruptEntry\@:
        SAVE_ALL_REGS

        ldr     lr, =InterruptExit  // Return to InterruptExit
        mov     x0, sp              // Argument to exception handler
        b       Exception_\handler  // Call handler, returning at InterruptExit

    .org .InterruptEntry\@ + 0x80	//  Did we overflow the vector entry slot?
.endm

.balign 8
InterruptExit:
    RESTORE_ALL_REGS

    eret

.balign 0x800
.global ExceptionVectorEL1
ExceptionVectorEL1:
    // Current EL with SP0
    EXCEPTION_ENTRY EL1t_SP0_Synchronous
    EXCEPTION_ENTRY EL1t_SP0_IRQ
    EXCEPTION_ENTRY EL1t_SP0_FIQ
    EXCEPTION_ENTRY EL1t_SP0_SystemError

    // Current EL with SPx
    EXCEPTION_ENTRY EL1h_SPx_Synchronous
    EXCEPTION_ENTRY EL1h_SPx_IRQ
    EXCEPTION_ENTRY EL1h_SPx_FIQ
    EXCEPTION_ENTRY EL1h_SPx_SystemError

    // Lower EL using aarch64
    EXCEPTION_ENTRY EL0_64_Synchronous
    EXCEPTION_ENTRY EL0_64_IRQ
    EXCEPTION_ENTRY EL0_64_FIQ
    EXCEPTION_ENTRY EL0_64_SystemError

    // Lower EL using aarch32
    EXCEPTION_ENTRY EL0_32_Synchronous
    EXCEPTION_ENTRY EL0_32_IRQ
    EXCEPTION_ENTRY EL0_32_FIQ
    EXCEPTION_ENTRY EL0_32_SystemError
