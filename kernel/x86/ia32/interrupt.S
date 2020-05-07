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


.macro INTERRUPT_ENTRY num
    .global interrupt_entry_\num
    interrupt_entry_\num:
        .if !(\num == 8 || (\num >= 10 && \num <= 14) || \num == 17 || \num == 30)
            pushl $0
        .endif

        pushl $\num
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
    pushl %ebp
    pushl %edi
    pushl %esi
    pushl %edx
    pushl %ecx
    pushl %ebx
    pushl %eax
    pushw %gs
    pushw %fs
    pushw %es
    pushw %ds

    # Sys V ABI requires DF to be clear on function entry
    cld

    pushl %esp          # Argument to interrupt_dispatch()
    call interrupt_dispatch
    addl $4, %esp       # Pop arguments


.global interrupt_exit
interrupt_exit:

    # Restore interrupt context
    popw %ds
    popw %es
    popw %fs
    popw %gs
    popl %eax
    popl %ebx
    popl %ecx
    popl %edx
    popl %esi
    popl %edi
    popl %ebp

    # Pop interrupt number and error code
    addl $8, %esp

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

#TODO: trap flag is not cleared, must be handled (likely in the debug exception handler)
#TODO: trap flag must also be restored when returning to usermode (use IRET)

    pushf       # Flags
    pushl %ebp  # User stack - esp
    pushl %edi  # arg5
    pushl %esi  # arg4
    pushl %edx  # arg3
    pushl %ecx  # arg2
    pushl %ebx  # arg1
    pushl %eax  # function

    # Sys V ABI requires DF to be clear on function entry
    cld

    # Call handler
    pushl %esp      # Param to SysCall()
    call SysCall    # Handle system call
    addl $4, %esp   # Remove param from stack

    popl %eax
    popl %ebx
    popl %ecx
    popl %edx
    popl %esi
    popl %edi
    popl %ecx       # User stack in ecx for sysexit
    popf

    movl g_vdso + 4, %edx # Always exit at vdso_sysexit

    sti             # Re-enable interrupts (sysexit won't do it for us)

    sysexit         # See ya!
