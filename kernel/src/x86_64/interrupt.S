# Copyright (c) 2023, Thierry Tremblay
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

.code64
.section .text

.macro SAVE_ALL_REGS
    pushq %r15
    pushq %r14
    pushq %r13
    pushq %r12
    pushq %r11
    pushq %r10
    pushq %r9
    pushq %r8
    pushq %rbp
    pushq %rdi
    pushq %rsi
    pushq %rdx
    pushq %rcx
    pushq %rbx
    pushq %rax
.endm

.macro RESTORE_ALL_REGS
    popq %rax
    popq %rbx
    popq %rcx
    popq %rdx
    popq %rsi
    popq %rdi
    popq %rbp
    popq %r8
    popq %r9
    popq %r10
    popq %r11
    popq %r12
    popq %r13
    popq %r14
    popq %r15
.endm

.macro EXCEPTION_ENTRY num handler
    .align 8
    .global InterruptEntry\num
    InterruptEntry\num:
        # Some exceptions don't push an error code, so we push a fake one: zero
        .if !(\num == 8 || (\num >= 10 && \num <= 14) || \num == 17 || \num == 30)
        pushq $0
        .endif

        # Save interrupt context
        SAVE_ALL_REGS

        # Call handler
        cld                     # Sys V ABI requires DF to be clear on function entry
        movq    %rsp, %rbx      # Save stack pointer into rbx
        movq    %rsp, %rdi      # Argument to exception handler
        andq    $-16, %rsp      # Align stack on 16 bytes as per Sys V ABI
        pushq   $InterruptExit0 # Return to InterruptExit
        jmp     \handler        # Call handler, returning at InterruptExit
.endm


.macro INTERRUPT_ENTRY num
    .align 8
    .global InterruptEntry\num
    InterruptEntry\num:
        pushq   $\num
        jmp     InterruptCommon
.endm

# CPU exceptions
EXCEPTION_ENTRY 0  ExceptionDivideError
EXCEPTION_ENTRY 1  ExceptionDebug
EXCEPTION_ENTRY 2  ExceptionNmi
EXCEPTION_ENTRY 3  ExceptionBreakpoint
EXCEPTION_ENTRY 4  ExceptionOverflow
EXCEPTION_ENTRY 5  ExceptionBoundRangeExceeded
EXCEPTION_ENTRY 6  ExceptionInvalidOpcode
EXCEPTION_ENTRY 8  ExceptionDoubleFault
EXCEPTION_ENTRY 10 ExceptionInvalidTss
EXCEPTION_ENTRY 11 ExceptionStackSegment
EXCEPTION_ENTRY 12 ExceptionStack
EXCEPTION_ENTRY 13 ExceptionGeneral
EXCEPTION_ENTRY 14 ExceptionPageFault
EXCEPTION_ENTRY 16 ExceptionFpu
EXCEPTION_ENTRY 17 ExceptionAlignment
EXCEPTION_ENTRY 18 ExceptionMachineCheck
EXCEPTION_ENTRY 19 ExceptionSimd

# External interrupts (IRQs)
.altmacro
.set i, 32
.rept 256-32
    INTERRUPT_ENTRY %i
    .set i, i+1
.endr

.align 8
InterruptCommon:

    # Kernel CS is set from an IDT entry
    # Kernel SS is set from an TSS entry

    # Note: we use interrupt gates to ensure interrupts are disabled on entry.

# TODO: properly handle NMI, MCE, #DB, ...

    # Save interrupt context
    SAVE_ALL_REGS

    # Call handler
    cld                         # Sys V ABI requires DF to be clear on function entry
    movq    %rsp, %rbx          # Save stack pointer into rbx
    movq    %rsp, %rdi          # Argument to InterruptDispatch()
    andq    $-16, %rsp          # Align stack on 16 bytes as per Sys V ABI
    call    InterruptDispatch   # Call handler

InterruptExit0:
    mov     %rbx, %rsp          # Restore stack pointer

.global InterruptExit
InterruptExit:

    # Restore interrupt context
    RESTORE_ALL_REGS

    # Pop error code / interrupt number
    addq    $8, %rsp

    iretq
