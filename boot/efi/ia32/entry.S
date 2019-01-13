/*
    Copyright (c) 2018, Thierry Tremblay
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



###############################################################################
#
# Program entry point
#
###############################################################################

.section .text
.code32

.global _start
_start:
    pushl %ebp
    movl %esp, %ebp

    # We need to calculate where ImageBase is (we are using PIC code)
    call 0f
0:  popl %eax                 # eax = PC at label 0
    addl $ImageBase-0b, %eax  # eax = ImageBase

    pushl %eax          # arg1 = ImageBase
    call _relocate      # Self-relocate this program
    testl %eax,%eax     # Check for success?
    jne .exit           # Error, get out

    call _init          # Initialize runtime

    pushl 12(%ebp)      # arg1 = EFI_HANDLE parameter
    pushl 8(%ebp)       # arg2 = EFI_SYSTEM_HANDLE* parameter
    call efi_main       # Launch bootloader

.exit:
    leave               # Restore stack and ebp
    ret                 # Bye bye


###############################################################################
#
# Trampoline to jump to the kernel
#
###############################################################################

.global jumpToKernel
jumpToKernel:

    pushl   %ebx            # Save ebx

    movl    %esp, %eax      # eax = old stack
    movl    8(%esp), %ebx   # ecx = BootInfo*
    movl    12(%esp), %ecx  # ebx = kernel entry point
    movl    16(%esp), %edx  # edx = new stack

    movl    %edx, %esp      # Switch to new stack
    pushl   %eax            # Save old stack

    pushl   %ebx            # BootInfo* parameter for kernel
    call    *%ecx           # Call kernel

    popl    %ecx            # Pop argument to kernel
    popl    %ecx            # ecx = old stack
    movl    %ecx, %esp      # Restore old stack

    popl    %ebx            # Restore ebx

    ret


###############################################################################
#
# Dummy relocation so that EFI will load us
#
###############################################################################

.section .data
dummy:
    .long   0

.section .reloc
    .long   dummy   # Page RVA
    .long   10      # Block Size (2*4+2)
    .word   0       # Reloc for dummy
