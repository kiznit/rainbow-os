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

.global JumpToUserMode
JumpToUserMode:

    movl 4(%esp), %edx  # edx = entry point in user space
    movl 8(%esp), %ebx  # ebx = user args
    movl 12(%esp), %ecx # ecx = user space stack

    movl $0x23, %eax    # Selector 0x20 (User Data) + RPL 3
    movw %ax, %ds
    movw %ax, %es

    # Setup "user args" as parameter to user function
    subl $8, %ecx       # Make room for parameter and return value
    movl $0, 0(%ecx)    # Fake return address (not stricly needed when spawning a new process, but needed when creating new threads)
    movl %ebx, 4(%ecx)  # First parameter = user args

    # Build a fake iret frame
    pushl %eax          # Selector 0x20 (User Data) + RPL 3
    pushl %ecx          # User space stack
    pushl $0x202        # eflags = interrupt enable + reserved bit
    pushl $0x1b         # Selector 0x18 (User Code) + RPL 3
    pushl %edx          # Entry point in user space

    iret
