# Copyright (c) 2024, Thierry Tremblay
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
# Kernel entry point
#
###############################################################################

.code64
.section .text

.global _start

_start:
    # Params
    #   rdi = BootInfo*

    cli                             # Disable interrupts
    cld                             # Clear direction flag

    # Firmwares can leave control registers in any state
    mov     $0x80000011, %rax       # PG + ET + PE
    mov     %rax, %cr0

    mov     $0x000006a0, %rax       # OSFXSR/SSE + OSXMMEXCPT + PGE + PAE
    mov     %rax, %cr4

    mov     $_boot_stack, %rsp      # Initialize stack

    # rdi holds boot info parameter
    call    _kernel_start           # Execute kernel, not expected to return

    # Hang if kernel unexpectedly returns
.halt:
    cli                             # Disable interrupts
    hlt
    jmp     .halt


###############################################################################
#
# Initial stack
#
###############################################################################

.section .bss

.global _boot_stack, _boot_stack_top;

.align 4096

_boot_stack_top:
.skip 128*1024
_boot_stack:
