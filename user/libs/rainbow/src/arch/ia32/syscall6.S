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
#  syscall6
#
###############################################################################

.section .text
.code32

.align 16

.global syscall6
syscall6:

    # System V ABI says: preserve these registers!
    pushl %ebx
    pushl %esi
    pushl %edi
    pushl %ebp

    movl 0x14(%esp), %eax   # Function
    movl 0x18(%esp), %ebx   # Param 1
    movl 0x1C(%esp), %ecx   # Param 2
    movl 0x20(%esp), %edx   # Param 3
    movl 0x24(%esp), %esi   # Param 4
    movl 0x28(%esp), %edi   # Param 5
    movl 0x2C(%esp), %ebp   # Param 6

    call *0xEFFFF000        # Entry point for system

    # Restore preserved registers
    popl %ebp
    popl %edi
    popl %esi
    popl %ebx

    # Return to caller
    ret
