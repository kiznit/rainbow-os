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
.code64

.global JumpToUserMode
JumpToUserMode:

    # rdi = entry point in user space
    # rsi = user args
    # rdx = user space stack

    movq $0x1b, %rax    # Selector 0x18 (User Data) + RPL 3
    movw %ax, %ds
    movw %ax, %es       # TODO: this is questionable
    movw %ax, %fs       # TODO: this is questionable
    movw %ax, %gs       # TODO: this is questionable

    # Build a fake iret frame
    pushq %rax          # Selector 0x18 (User Data) + RPL 3
    pushq %rdx          # User space stack
    pushq $0x200        # rflags = interrupt enable
    pushq $0x23         # Selector 0x20 (User Code) + RPL 3
    pushq %rdi          # Entry point in user space

    # Setup "user args" as parameter to user function
    movq %rsi, %rdi     # First parameter = user args

    iretq
