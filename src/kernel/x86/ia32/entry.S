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
.code32

.equ SYSCALL_MAX, 9

# GDT selectors
.equ GDT_KERNEL_CODE,   0x08
.equ GDT_KERNEL_DATA,   0x10
.equ GDT_USER_CODE,     0x1b
.equ GDT_USER_DATA,     0x23
.equ GDT_CPU_DATA,      0x30


.macro SAVE_ALL_REGS
    pushl %gs
    pushl %fs
    pushl %es
    pushl %ds
    pushl %edx  # Syscall arg3
    pushl %ecx  # Syscall arg2
    pushl %ebx  # Syscall arg1
    pushl %eax  # Syscall function number and return value
    pushl %ebp  # Syscall user stack - arg 6 at %ebp(0)
    pushl %edi  # Syscall arg5
    pushl %esi  # Syscall arg4
.endm


.macro RESTORE_ALL_REGS
    popl %esi
    popl %edi
    popl %ebp
    popl %eax
    popl %ebx
    popl %ecx
    popl %edx
    popl %ds
    popl %es
    popl %fs
    popl %gs
.endm


.macro SET_DS_FS
    movl $GDT_KERNEL_DATA, %esi
    movl $GDT_CPU_DATA, %edi
    movl %esi, %ds
    movl %edi, %fs
.endm


.macro EXCEPTION_ENTRY num handler
    .align 4
    .global interrupt_entry_\num
    interrupt_entry_\num:
        # Some exceptions don't push an error code, so we push a fake one: zero
        .if !(\num == 8 || (\num >= 10 && \num <= 14) || \num == 17 || \num == 30)
            pushl $0
        .endif

        # Save interrupt context
        SAVE_ALL_REGS

        # Load kernel segments
        SET_DS_FS

        # Call handler
        cld                     # Sys V ABI requires DF to be clear on function entry

        movl %esp, %eax         # 1st argument: InterruptContext*
        .if (\num == 14)
            movl %cr2, %edx     # 2nd argument: page fault address
        .endif

        call \handler           # Call handler

        jmp interrupt_exit
.endm


.macro INTERRUPT_ENTRY num
    .align 4
    .global interrupt_entry_\num
    interrupt_entry_\num:
        pushl $\num
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


.align 4
interrupt_common:

    # Kernel CS is set from an IDT entry
    # Kernel SS is set from an TSS entry

    # Note: we use interrupt gates to ensure interrupts are disabled on entry.

# TODO: properly handle NMI, MCE, #DB, ...

    # Save interrupt context
    SAVE_ALL_REGS

    # Load kernel segments
    SET_DS_FS

    # Call handler
    cld                     # Sys V ABI requires DF to be clear on function entry
    movl %esp, %eax         # 1st argument: InterruptContext*
    call interrupt_dispatch # Call handler

.align 4
.global interrupt_exit
interrupt_exit:

    # Restore interrupt context
    RESTORE_ALL_REGS

    # Pop error code / interrupt number
    addl $4, %esp

    iret



.section .vdso

.align 4
.global vdso_syscall, vdso_syscall_exit
vdso_syscall:
    pushl   %ecx        # Will be clobbered for sysexit (ecx = user esp)
    pushl   %edx        # Will be clobbered for sysexit (edx = user eip)
    pushl   %ebp        # Save ebp as we will use it to hold user esp on entry
    movl    %esp, %ebp  # Store esp in ebp so the kernel knows where the user stack is
    sysenter            # Call kernel
    nop                 # Align vdso_syscall_exit on 4 bytes
vdso_syscall_exit:
    popl    %ebp
    popl    %edx
    popl    %ecx
    retl                # Return to caller



.section .text

.align 4
.global sysenter_entry
sysenter_entry:

    # Kernel CS is set from MSR_SYSENTER_CS
    # Kernel SS is set from MSR_SYSENTER_CS + 8
    # Kernel EIP is set from MSR_SYSENTER_EIP
    # Kernel ESP is set from MSR_SYSENTER_ESP

    # syscall parameters are stored in ebx, ecx, edx, esi, edi
    # user stack in %ebp

    # Note: sysenter will clear VM, IF and RF flags.
    # This ensures that interrupts are disable on entry.

# TODO: properly handle NMI, MCE, #DB, ...

#TODO: trap flag is not cleared, must be handled (likely in the debug exception handler)
#TODO: trap flag must also be restored when returning to usermode (use IRET)

    # Build an iret frame so that we can return to user space using iret when needed
# TODO: is this ever needed in ia32 with sysenter/sysexit?

    # Save interrupt context
    pushl $GDT_USER_DATA    # User ds
    pushl %ebp              # User esp
    pushfl                  # User eflags
    pushl $GDT_USER_CODE    # User cs
    pushl g_vdso + 4        # User eip: vdso_sysexit (in user space)

    pushl %ebx              # Syscall number

    SAVE_ALL_REGS

    # Set segments
    SET_DS_FS

    # Call handler
    cld                     # Sys V ABI requires DF to be clear on function entry
    cmpl $SYSCALL_MAX, %ebx # Syscall function number valid?
    ja sysenter_error       # N: handle invalid syscall function number
    movl syscall_table(, %ebx, 4), %ebx
    call *%ebx              # Handle syscall

sysenter_exit:

    # Store result (eax) on stack
    movl %eax, 3*4(%esp)

    # Uncomment to test iret exit path
    #jmp interrupt_exit

    RESTORE_ALL_REGS

    addl $4, %esp   # Pop syscall number

    popl %edx       # Return address goes into edx for sysexit
    addl $4, %esp   # Skip saved cs
    btrl $9, (%esp) # Clear IF flag in saved eflags. We do want to be interrupted with an active user stack
    popf            # Restore flags except IF
    popl %ecx       # User stack goes into ecx for sysexit
    popl %ebp       # Pop saved ss - use this instead of addl so that we don't change eflags
                    # Thrashing ebp is fine, vdso_sysenter saves it and we restore it in vdso_sysexit

    sti             # Re-enable interrupts - STI guarantees the next instruction won't get interrupted.
    sysexit         # See ya!


sysenter_error:
    movl $-1, %eax
    jmp  sysenter_exit
