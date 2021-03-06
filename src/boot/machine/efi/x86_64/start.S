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


###############################################################################
#
# Program entry point
#
###############################################################################

.section .text
.code64

.global _start
_start:
    pushq   %rdi            # Save rdi for caller
    pushq   %rsi            # Save rsi for caller

    pushq   %rbp            # Setup stack frame, align stack to 16 bytes
    movq    %rsp, %rbp

    pushq   %rdx            # EFI_SYSTEM_TABLE*
    pushq   %rcx            # EFI_HANDLE

    lea     ImageBase(%rip), %rdi   # arg1 = ImageBase
    call    _relocate       # Self-relocate this program
    testq   %rax,%rax       # Check for success?
    jne     .exit           # Error, get out

    popq    %rdi            # arg1 = EFI_HANDLE
    popq    %rsi            # arg2 = EFI_SYSTEM_TABLE*
    call    _start_efi      # Execute bootloader, not expected to return

.exit:
    leave                   # Restore stack and rbp
    popq    %rsi            # Restore rsi
    popq    %rdi            # Restore rdi
    ret                     # Bye bye


###############################################################################
#
# Trampoline to jump to the kernel
#
###############################################################################

.global KernelTrampoline, KernelTrampolineStart, KernelTrampolineEnd

KernelTrampolineStart:

    # rdi = kernel entry point
    # rsi = BootInfo*
    # rdx = page tables

    # Enable PGE
    movq    %cr4, %rax
    btsq    $7, %rax
    movq    %rax, %cr4

    # Set page tables
    movq    %rdx, %cr3

    # Jump to kernel
    xchgq   %rsi, %rdi
    call    *%rsi           # Call kernel

.halt:
    cli
    hlt
    jmp .halt

KernelTrampolineEnd:


###############################################################################
#
# Dummy relocation so that EFI will load us
#
###############################################################################

.section .data
dummy:
    .long   0

.section .reloc, "a"
label:
    .long   dummy-label     # Page RVA
    .long   10              # Block Size (2*4+2)
    .word   0               # Reloc for dummy
