# Copyright (c) 2018, Thierry Tremblay
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
#   CallBios
#
#   This code is to be copied to 0x8000 so that it can be executed.
#   Real mode stack will be initialized right after the trampoline code.
#
###############################################################################

.section .text
.code32

.global BiosTrampolineStart, BiosTrampolineEnd, BiosStackTop

.global CallBios
.type CallBios,@function

.align 16

BiosTrampolineStart:

CallBios:
    # System V ABI says: preserve these registers!
    pushl %ebx
    pushl %esi
    pushl %edi
    pushl %ebp

    # Patch 'int' instruction below
    movl 20(%esp), %eax
    movb %al, intInstruction - CallBios + 0x8000 + 1

    # Save stack pointer so we can get back from real mode
    movl %esp, BiosStackTop - CallBios + 0x8000 - 4

    # Copy input BiosRegisters structure to real mode stack (44 bytes)
    movl $11, %ecx
    movl 24(%esp), %esi
    movl $(BiosStackTop - CallBios + 0x8000 - 48), %edi
    rep movsl

    cli                         # Disable interrupts
    lidt idt_descriptor16       # Restore real-mode IDT

    # Jump to 16-bit protected code
    ljmp $0x08, $(protectedMode16 - BiosTrampolineStart + 0x8000)


.code16
protectedMode16:
    # Disable protected mode
    movl %cr0, %eax
    btrl $0, %eax               # Clear PE bit (0)
    movl %eax, %cr0

    # Jump to real mode
    ljmp $0x00, $(realMode - BiosTrampolineStart + 0x8000)


realMode:

    # Setup real mode stack
    xorw %ax, %ax
    movw %ax, %ss
    movl $(BiosStackTop - BiosTrampolineStart + 0x8000 - 48), %esp

    sti                         # Enable interrupts

    # Load registers (caller's BiosRegisters)
    popw %ds
    popw %es
    popw %fs
    popw %gs
    popl %eax                   # Ignore eflags
    popal

     # Call the interrupt
intInstruction:
    int $0xFF                   # Interrupt number is modified above

    # Save registers (to be copied to the caller's BiosRegisters)
    pushal
    pushfl
    pushw %gs
    pushw %fs
    pushw %es
    pushw %ds

    cli                         # Disable interrupts
    cld                         # Clear direction flag

    # Go back to protected mode
    movl %cr0, %eax
    btsl $0, %eax               # Set PE bit (0)
    movl %eax, %cr0

    ljmpl $0x18, $(protectedMode)


.code32
protectedMode:

    # Load protected mode data segments
    movl $0x20, %eax
    movl %eax, %ds
    movl %eax, %es
    movl %eax, %fs
    movl %eax, %gs

    # Restore protected mode stack pointer
    movl %eax, %ss
    movl BiosStackTop - CallBios + 0x8000 - 4, %esp

    # Copy real mode registers back to output BiosRegisters structure (44 bytes)
    movl $11, %ecx
    movl 28(%esp), %edi
    movl $(BiosStackTop - CallBios + 0x8000 - 48), %esi
    rep movsl

    # Return 'eax' to the caller
    mov -4(%edi), %eax

    # Restore preserved registers
    popl %ebp
    popl %edi
    popl %esi
    popl %ebx

    # Return to caller
    ret


    .size CallBios, . - CallBios

BiosTrampolineEnd:

    .align 16

BiosStack:

BiosStackTop = BiosStack + 4096




###############################################################################
#
#   Data
#
###############################################################################

.section .rodata

idt_descriptor16:
    .align 16
    .short 0x3FF    # Limit = 0x400 = 256 * 4 bytes / interrupt
    .long 0         # Base = 0
