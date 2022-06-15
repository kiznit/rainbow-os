# Copyright (c) 2022, Thierry Tremblay
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

.equ CR4_PGE, 7
.equ IA32_EFER, 0xc0000080
.equ IA32_EFER_NX, 11

.global KernelTrampolineStart, KernelTrampolineEnd

.align 16
KernelTrampolineStart:
    # Params (Microsoft x64 calling convention)
    #   rcx = BootInfo*
    #   rdx = kernel entry point
    #   r8  = page table

    mov     %rcx, %rdi  # Save BootInfo* and set as param 1 for kernel entry point
    mov     %rdx, %rsi  # Save kernel entry point

    # Enable PGE (Page Global Enable)
    mov     %cr4, %rax
    bts     $CR4_PGE, %rax
    mov     %rax, %cr4

    # Enable NX (No-eXecute)
    mov     $IA32_EFER, %ecx
    rdmsr
    bts     $IA32_EFER_NX, %eax
    wrmsr

    # Set page table
    mov     %r8, %cr3

    # Jump to kernel
    jmp     *%rsi       # Jump to kernel

KernelTrampolineEnd:
