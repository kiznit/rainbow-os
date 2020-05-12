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


.macro SAVE_ALL_REGS
    pushw %gs
    pushw %fs
    pushw %es
    pushw %ds
    pushl %eax  # Syscall function and return value
    pushl %ebp  # Syscall user stack
    pushl %edi  # Syscall arg5
    pushl %esi  # Syscall arg4
    pushl %edx  # Syscall arg3
    pushl %ecx  # Syscall arg2
    pushl %ebx  # Syscall arg1
.endm


.macro RESTORE_ALL_REGS
    popl %ebx
    popl %ecx
    popl %edx
    popl %esi
    popl %edi
    popl %ebp
    popl %eax
    popw %ds
    popw %es
    popw %fs
    popw %gs
.endm


.macro EXCEPTION_ENTRY num handler
    .align 4
    .global interrupt_entry_\num
    interrupt_entry_\num:
        .if !(\num == 8 || (\num >= 10 && \num <= 14) || \num == 17 || \num == 30)
            pushl $0
        .endif

        # Save interrupt context
        SAVE_ALL_REGS

        # Call handler
        cld                 # Sys V ABI requires DF to be clear on function entry
        .if (\num == 14)    # Page fault?
            movl %cr2, %eax # Get page fault address
            movl %esp, %ebx # InterruptContext*
            pushl %eax      # Second argument = page fault address
            pushl %ebx      # First argument = InterruptContext*
            call \handler   # Call handler
            addl $8, %esp   # Pop argumnets
        .else
            pushl %esp      # Argument to interrupt_dispatch()
            call \handler   # Call handler
            addl $4, %esp   # Pop arguments
        .endif

        jmp interrupt_exit
.endm


.macro INTERRUPT_ENTRY num
    .align 4
    .global interrupt_entry_\num
    interrupt_entry_\num:
        pushl $\num
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


.align 4
interrupt_entry:

    # Save interrupt context
    SAVE_ALL_REGS

    # Call handler
    cld                     # Sys V ABI requires DF to be clear on function entry
    pushl %esp              # Argument to interrupt_dispatch()
    call interrupt_dispatch # Call handler
    addl $4, %esp           # Pop arguments


.align 4
.global interrupt_exit
interrupt_exit:

    # Restore interrupt context
    RESTORE_ALL_REGS

    # Pop error code / interrupt number
    addl $4, %esp

    iret



.section .vdso

.global vdso_sysenter
vdso_sysenter:
    pushl   %ecx        # Will be clobbered for sysexit (ecx = user esp)
    pushl   %edx        # Will be clobbered for sysexit (edx = user eip)
    pushl   %ebp        # Save ebp as we will use it to hold user esp on entry
    movl    %esp, %ebp  # Store esp in ebp so the kernel knows where the user stack is
    sysenter            # Call kernel

.global vdso_sysexit
vdso_sysexit:
    popl    %ebp
    popl    %edx
    popl    %ecx
    retl                # Return to caller


.section .text

.global sysenter_entry
sysenter_entry:

    # Note: sysenter will clear VM, IF and RF flags.
    # This means that interrupts are disabled.

    # syscall parameters are stored in ebx, ecx, edx, esi, edi

#TODO: trap flag is not cleared, must be handled (likely in the debug exception handler)
#TODO: trap flag must also be restored when returning to usermode (use IRET)

    # Save interrupt context
    pushf       # Flags
    pushl %eax  # Syscall function number
    pushl %ebp  # User stack - esp
    pushl %edi  # arg5
    pushl %esi  # arg4
    pushl %edx  # arg3
    pushl %ecx  # arg2
    pushl %ebx  # arg1

    # Sys V ABI requires DF to be clear on function entry
    cld

    # Call handler
    cmpl $5, %eax   # Syscall function number valid?
    jae error       # N: handle invalid syscall function number
    movl syscall_table(, %eax, 4), %eax
    call *%eax      # Handle syscall

exit_sysenter:
    popl %ebx
    popl %ecx
    popl %edx
    popl %esi
    popl %edi
    popl %ecx       # User stack in ecx for sysexit
    addl $4, %esp   # Pop old %eax (but preserve current %eax, it has the return value)

    movl g_vdso + 4, %edx # Always exit at vdso_sysexit

    # Restore flags except IF. We do this because STI guarantees the next instruction
    # won't get interrupted, but POPF doesn't.
    btrl $9, (%esp)
    popf

    sti             # Re-enable interrupts (sysexit won't do it for us)
    sysexit         # See ya!


error:
    movl $-1, %eax
    jmp  exit_sysenter
