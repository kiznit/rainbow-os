# Copyright (c) 2018, Thierry Tremblay
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
.code64


.macro INTERRUPT_ENTRY num
    .global interrupt_entry_\num
    interrupt_entry_\num:
        .if !(\num == 8 || (\num >= 10 && \num <= 14) || \num == 17 || \num == 30)
            pushq $0
        .endif

        pushq $\num
        jmp interrupt_entry
.endm


.altmacro
.set i, 0
.rept 256
    INTERRUPT_ENTRY %i
    .set i, i+1
.endr


interrupt_entry:

    # Save interrupt context
#TODO: only save volatile + segment registers?
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
    pushw %gs
    pushw %fs
    subq $4, %rsp
    movw %es, 2(%rsp)
    movw %ds, (%rsp)

    # Page fault handler needs CR2, others don't...
    movq %cr2, %rax
    pushq %rax

    # Sys V ABI requires DF to be clear on function entry
    cld

    movq %rsp, %rbx     # Save stack pointer into ebx
    movq %rsp, %rdi     # Argument to interrupt_dispatch()
    andq $-15, %rsp     # Align stack on 16 bytes as per Sys V ABI
    call interrupt_dispatch
    mov %rbx, %rsp      # Restore stack pointer


.global interrupt_exit
interrupt_exit:

    # Restore interrupt context
    addq $8, %rsp        # CR2
    movw (%rsp), %ds
    movw 2(%rsp), %es
    addq $4, %rsp
    popw %fs
    popw %gs
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

    # Pop interrupt number and error code
    addq $16, %rsp

    iretq
