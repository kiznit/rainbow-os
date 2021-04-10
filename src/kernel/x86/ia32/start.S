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
# Kernel entry point
#
###############################################################################

.section .text
.code32

.global _start

_start:
    # BootInfo* on stack (%esp + 4)

    cli                             # Disable interrupts
    cld                             # Clear direction flag

    # Firmwares can leave control registers in any state
    movl    $0x80000011, %eax       # PG + ET + PE
    movl    %eax, %cr0

    movl    $0x000006a0, %eax       # OSFXSR/SSE + OSXMMEXCPT + PGE + PAE
    movl    %eax, %cr4

    movl    4(%esp), %ebx           # ebx = BootInfo*
    movl    $_boot_stack, %esp      # Initialize stack

    pushl   %ebx                    # BootInfo* parameter to _start_kernel()
    call    _start_kernel           # Execute kernel, not expected to return
    addl    $4, %esp                # Remove argument to _start_kernel()

    # Hang if kernel unexpectedly returns
.global cpu_halt
cpu_halt:
    cli
    hlt
    jmp     cpu_halt


###############################################################################
#
# Boot stack
#
###############################################################################

.section .bss
.align 4096

.global _boot_stack, _boot_stack_top;

_boot_stack_top:
.skip 128*1024
_boot_stack:
