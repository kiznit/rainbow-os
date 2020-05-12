# Copyright (c) 2020, Thierry Tremblay
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
    .global interrupt_entry_\num
    interrupt_entry_\num:
        .if !(\num == 8 || (\num >= 10 && \num <= 14) || \num == 17 || \num == 30)
            pushq $0
        .endif

        # Save interrupt context
        SAVE_ALL_REGS

        # Call handler
        cld                 # Sys V ABI requires DF to be clear on function entry
        movq %rsp, %rdi     # Argument to interrupt_dispatch() = InterruptContext*
        .if (\num == 14)    # Page fault?
            movq %cr2, %rsi # Second argument: page fault address
        .endif
        call \handler       # Call handler

        jmp interrupt_exit
.endm


.macro INTERRUPT_ENTRY num
    .align 8
    .global interrupt_entry_\num
    interrupt_entry_\num:
        pushq $\num
        jmp interrupt_entry
.endm


EXCEPTION_ENTRY 0  exception_divide_error
EXCEPTION_ENTRY 1  exception_debug
EXCEPTION_ENTRY 2  exception_nmi
EXCEPTION_ENTRY 3  exception_breakpoint
EXCEPTION_ENTRY 4  exception_overflow
EXCEPTION_ENTRY 5  exception_bound_range_exceeded
EXCEPTION_ENTRY 6  exception_invalid_opcode
EXCEPTION_ENTRY 8  exception_double_fault
EXCEPTION_ENTRY 10 exception_invalid_tss
EXCEPTION_ENTRY 11 exception_stack_segment
EXCEPTION_ENTRY 12 exception_stack
EXCEPTION_ENTRY 13 exception_general
EXCEPTION_ENTRY 14 exception_page_fault
EXCEPTION_ENTRY 16 exception_fpu
EXCEPTION_ENTRY 17 exception_alignment
EXCEPTION_ENTRY 18 exception_machine_check
EXCEPTION_ENTRY 19 exception_simd


.altmacro
.set i, 32
.rept 256-32
    INTERRUPT_ENTRY %i
    .set i, i+1
.endr


.align 8
interrupt_entry:

# TODO: properly handle NMI, MCE, #DB, ...

    # Save interrupt context
    SAVE_ALL_REGS

    # Call handler
    cld                     # Sys V ABI requires DF to be clear on function entry
    movq %rsp, %rbx         # Save stack pointer into rbx
    movq %rsp, %rdi         # Argument to interrupt_dispatch()
    andq $-15, %rsp         # Align stack on 16 bytes as per Sys V ABI
    call interrupt_dispatch # Call handler
    mov %rbx, %rsp          # Restore stack pointer


.align 8
.global interrupt_exit
interrupt_exit:

    # Restore interrupt context
    RESTORE_ALL_REGS

    # Pop error code / interrupt number
    addq $8, %rsp

    iretq



.section .text

.global syscall_entry
syscall_entry:

    # Note: we configured MSR_FMASK so that syscall will clear VM, IF and RF flags.
    # This means that interrupts are disabled.

    # syscall parameters are stored in rdi, rsi, rdx, r10, r8, r9
    # rcx has return address (but we don't care?)
    # r11 has rflags before masking

# TODO: properly handle NMI, MCE, #DB, ...

    # Setup kernel stack
    movq %rsp, g_userRsp
    movq g_kernelRsp, %rsp

    pushq %rcx  # User rip
    pushq %r11  # User rflags
    pushq %r9   # arg6
    pushq %r8   # arg5
    pushq %r10  # arg4
    pushq %rdx  # arg3
    pushq %rsi  # arg2
    pushq %rdi  # arg1
    pushq %rax  # function

    # Call handler
    cmpq $5, %rax   # Syscall function number valid?
    jae error       # N: handle invalid syscall function number
    movq %r10, %rcx # Arg 4 goes into rcx (Sys V ABI)
    movq syscall_table(, %rax, 8), %rax
    call *%rax      # Handle syscall

exit_sysret:
    addq $8, %rsp   # Pop old %rax (but preserve current %rax, it has the return value)
    popq %rdi
    popq %rsi
    popq %rdx
    popq %r10
    popq %r8
    popq %r9
    popq %r11   # User rflags
    popq %rcx   # User rip

    # Make sure rcx is canonical
    # We are returning to user space, so we can just mask out higher bits
# TODO: use iret return path instead of doing this...
    movq $0x00007FFFFFFFFFFF, %rsp
    andq %rsp, %rcx

    # Restore user stack
    movq g_userRsp, %rsp

    sysretq     # See ya!


error:
    movq $-1, %rax
    jmp  exit_sysret
