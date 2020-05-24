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

.equ SYSCALL_MAX, 8

# GDT selectors
.equ GDT_KERNEL_CODE,   0x08
.equ GDT_KERNEL_DATA,   0x10
.equ GDT_USER_CODE,     0x1b
.equ GDT_USER_DATA,     0x23
.equ GDT_PER_CPU,       0x30


.macro SAVE_ALL_REGS skip_gs=0
.if \skip_gs == 0
    pushl %gs
.endif
    pushl %fs
    pushl %es
    pushl %ds
    pushl %eax  # Syscall function number and return value
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
    popl %ds
    popl %es
    popl %fs
    popl %gs
.endm


.macro SET_DS_GS
    movl $GDT_KERNEL_DATA, %ebx
    movl $GDT_PER_CPU, %ecx
    movl %ebx, %ds
    movl %ecx, %gs
.endm


.macro EXCEPTION_ENTRY num handler
    .align 4
    .global interrupt_entry_\num
    interrupt_entry_\num:
        .if !(\num == 8 || (\num >= 10 && \num <= 14) || \num == 17 || \num == 30)
            pushl $0
        .endif
        pushl $\handler
        jmp interrupt_common
.endm


.macro INTERRUPT_ENTRY num
    .align 4
    .global interrupt_entry_\num
    interrupt_entry_\num:
        pushl $\num
        pushl $interrupt_dispatch
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


#TODO: is it silly to have this trampoline? the alternative is to expand the page fault entry point
.align 4
exception_page_fault_trampoline:
    movl %cr2, %eax             # Get page fault address
    pushl %eax                  # Second argument = page fault address
    pushl 8(%esp)               # First argument = InterruptContext*
    call exception_page_fault   # Call handler
    add $16, %esp               # Pop page fault address, context, return address, context
    jmp interrupt_exit


.align 4
interrupt_common:

    # Kernel CS is set from an IDT entry
    # Kernel SS is set from an TSS entry

    # Note: we use interrupt gates to ensure interrupts are disabled on entry.

# TODO: properly handle NMI, MCE, #DB, ...

    # Save interrupt context
    SAVE_ALL_REGS 1

    # Handler is stored in gs's slot, fix things
    movl 40(%esp), %edx     # edx = handler
    movw %gs, 40(%esp)      # save gs

    # Set segments
    SET_DS_GS

    # Call handler
    cld                     # Sys V ABI requires DF to be clear on function entry
    pushl %esp              # Argument to interrupt_dispatch()
    call *%edx              # Call handler
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

.align 4
.global vdso_sysenter
vdso_sysenter:
    pushl   %ecx        # Will be clobbered for sysexit (ecx = user esp)
    pushl   %edx        # Will be clobbered for sysexit (edx = user eip)
    pushl   %ebp        # Save ebp as we will use it to hold user esp on entry
    movl    %esp, %ebp  # Store esp in ebp so the kernel knows where the user stack is
    sysenter            # Call kernel

.align 4
.global vdso_sysexit
vdso_sysexit:
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
    btsl $9, (%esp)         # Re-enable interrupts on exit
    pushl $GDT_USER_CODE    # User cs
    pushl g_vdso + 4        # User eip: vdso_sysexit (in user space)

    pushl %eax              # Syscall number

    SAVE_ALL_REGS

    # Set segments
    SET_DS_GS

    # Call handler
    cld                     # Sys V ABI requires DF to be clear on function entry
    cmpl $SYSCALL_MAX, %eax # Syscall function number valid?
    ja sysenter_error       # N: handle invalid syscall function number
    movl syscall_table(, %eax, 4), %eax
    call *%eax              # Handle syscall

sysenter_exit:

    # Store result (eax) on stack
    movl %eax, 6*4(%esp)

    # Uncomment to test iret exit path
    #jmp interrupt_exit

    RESTORE_ALL_REGS

    addl $4, %esp   # Pop syscall number

    popl %edx       # Return address goes into edx for sysexit
    addl $4, %esp   # Skip saved cs
    btrl $9, (%esp) # Restore flags except IF. We do this because STI guarantees the next...
    popf            # ...instruction won't get interrupted, but POPF doesn't.
    popl %ecx       # User stack goes into ecx for sysexit
    popl %ebp       # Pop saved ss - use this instead of addl so that we don't change eflags
                    # Thrashing ebp is fine, vdso_sysenter saves it and we restore it in vdso_sysexit

    sti             # Re-enable interrupts (sysexit won't do it for us)
    sysexit         # See ya!


sysenter_error:
    movl $-1, %eax
    jmp  sysenter_exit
