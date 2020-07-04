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

.equ SYSCALL_MAX, 6

# Per-cpu data offsets
.equ USER_STACK, 8
.equ KERNEL_STACK, 16

# GDT selectors
.equ GDT_KERNEL_CODE,   0x08
.equ GDT_KERNEL_DATA,   0x10
.equ GDT_USER_CODE,     0x23
.equ GDT_USER_DATA,     0x1b
.equ GDT_PER_CPU,       0x30


.macro SAVE_ALL_REGS skip_r15=0
.if \skip_r15 == 0
    pushq %r15
.endif
    pushq %r14
    pushq %r13
    pushq %r12
    pushq %r11  # Syscall user rflags
    pushq %r10  # Syscall arg4
    pushq %r9   # Syscall arg6
    pushq %r8   # Syscall arg5
    pushq %rbp
    pushq %rdi  # Syscall arg1
    pushq %rsi  # Syscall arg2
    pushq %rdx  # Syscall arg3
    pushq %rcx  # Syscall user rip
    pushq %rbx
    pushq %rax  # Syscall function number and return value
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
        pushq $\handler
        jmp interrupt_common
.endm


.macro INTERRUPT_ENTRY num
    .align 8
    .global interrupt_entry_\num
    interrupt_entry_\num:
        pushq $\num
        pushq $interrupt_dispatch
        jmp interrupt_common
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
EXCEPTION_ENTRY 14 exception_page_fault_trampoline
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
exception_page_fault_trampoline:
    movq %cr2, %rsi             # Second argument: page fault address
    jmp  exception_page_fault   # Call handler


.align 8
interrupt_common:

    # Kernel CS is set from an IDT entry
    # Kernel SS is set from an TSS entry

    # Note: we use interrupt gates to ensure interrupts are disabled on entry.

# TODO: properly handle NMI, MCE, #DB, ...

# TODO: since interrupts are always disabled in kernel mode, is this test still relevant?
    # If kernel code was interrupted, skip swapgs
    # https://www.kernel.org/doc/Documentation/x86/entry_64.txt
    testl $3, 24(%rsp)      # Was kernel code (DPL = 0) interrupted?
    jz 1f                   # Yes: skip swapgs
    swapgs                  # Switch to kernel GS
1:

    # Save interrupt context
    SAVE_ALL_REGS 1

    # Handler is stored in r15's slot, fix things
    movq 14*8(%rsp), %rcx   # rcx = handler
    movq %r15, 14*8(%rsp)   # save r15

    # Call handler
    cld                     # Sys V ABI requires DF to be clear on function entry
    movq %rsp, %rbx         # Save stack pointer into rbx
    movq %rsp, %rdi         # Argument to interrupt_dispatch()
    andq $-15, %rsp         # Align stack on 16 bytes as per Sys V ABI
    call *%rcx              # Call handler
    mov %rbx, %rsp          # Restore stack pointer


.align 8
.global interrupt_exit
interrupt_exit:

    # Restore interrupt context
    RESTORE_ALL_REGS

    # Pop error code / interrupt number
    addq $8, %rsp

    # Make sure interrupts are disabled before calling swapgs

# TODO: since interrupts are always disabled in kernel mode, is this test still relevant?
    # If returning to kernel code, skip swapgs
    testl $3, 8(%rsp)       # Was kernel code (DPL = 0) interrupted?
    jz 1f                   # Yes: skip swapgs
    swapgs                  # Switch to kernel GS
1:

    iretq



.section .vdso

.align 8
.global vdso_syscall, vdso_syscall_exit
vdso_syscall:
    syscall             # Call kernel
vdso_syscall_exit:
    retq                # Return to caller



.section .text

.align 8
.global syscall_entry
syscall_entry:

    # Kernel CS is set from MSR_STAR
    # Kernel SS is set from MSR_STAR
    # Kernel EIP is set from MSR_LSTAR
    # Kernel ESP is NOT set

    # syscall parameters are stored in rdi, rsi, rdx, r10, r8, r9
    # rcx has return address
    # r11 has rflags before masking

    # Note: we use interrupt gates to ensure interrupts are disabled on entry.

    # Note: we configured MSR_FMASK so that syscall will clear VM, IF and RF flags.
    # This ensures that interrupts are disable on entry.

# TODO: properly handle NMI, MCE, #DB, ...

    # Make sure interrupts are disabled before calling swapgs
    swapgs

    # Setup kernel stack
    movq %rsp, %gs:USER_STACK
    movq %gs:KERNEL_STACK, %rsp

    # Save interrupt context
    pushq $GDT_USER_DATA    # User ss
    pushq %gs:(USER_STACK)  # User rsp
    pushq %r11              # User rflags
    pushq $GDT_USER_CODE    # User cs
    pushq %rcx              # User rip

    pushq %rax              # Syscall number

    SAVE_ALL_REGS

    cmpq $SYSCALL_MAX, %rax # Syscall function number valid?
    ja syscall_error        # N: handle invalid syscall function number
    movq %r10, %rcx         # Arg 4 goes into rcx (Sys V ABI)
    movq syscall_table(, %rax, 8), %rax
    call *%rax              # Handle syscall

syscall_exit:

    # Store result (rax) on stack
    movq %rax, (%rsp)

    # Uncomment to test iret exit path
    #jmp interrupt_exit

    RESTORE_ALL_REGS

    addq $8, %rsp       # Pop syscall number

    popq %rcx           # Return address goes into rcx for sysret

    # Make sure rcx is canonical
    # We are returning to user space, so we can just mask out higher bits
    # TODO: use iret return path if rcx is not canonical?

    addq $8, %rsp       # Skip saved cs             # TODO: do not ignore, check and use iret if needed
    popq %r11           # User rflags goes into r11 for sysret

    # Make sure interrupts are disabled before loading user's rsp and calling swapgs
    # Currently this is true since interrupts are disabled at all time in kernel mode.

    movq (%rsp), %rsp   # Restore user stack - Can't use %gs:(USER_STACK)! No guarantee this is the same thread!
    swapgs
    sysretq             # See ya!


syscall_error:
    movq $-1, %rax
    jmp  syscall_exit
