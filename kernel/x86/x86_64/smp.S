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
#   SMP - Trampoline to start application processors (APs)
#
#   This code is to be copied in low memory (< 1 MB) so that it can be executed.
#
###############################################################################

# struct TrampolineContext
.equ PARAM_LOCATION,    0x0F00
.equ PARAM_FLAG,        PARAM_LOCATION + 0x00
.equ PARAM_CR3,         PARAM_LOCATION + 0x04
.equ PARAM_STACK,       PARAM_LOCATION + 0x08
.equ PARAM_ENTRY,       PARAM_LOCATION + 0x10
.equ PARAM_CPU,         PARAM_LOCATION + 0x18


.section .text
.code16

.global SmpTrampolineStart, SmpTrampolineEnd

SmpTrampolineStart:

    cli     # Disable interrupts
    cld     # Clear direction flag

    # On entry, CS = non-zero, DS = 0x0000
    xor     %ebx, %ebx              # Clear upper 16 bits to calculate SmpTrampolineStart below
    mov     %cs, %bx                # bx = cs
    mov     %bx, %ds                # ds = cs = non-zero
    shl     $4, %ebx                # ebx = address of SmpTrampolineStart

    # Notify main processor that the trampoline started execution
    movl    $1, PARAM_FLAG

    # Calculate GDT address and store it in GDT_PTR
    leal    (GDT - SmpTrampolineStart), %eax
    addl    %ebx, %eax
    movl    %eax, (GDT_PTR + 2 - SmpTrampolineStart)

    # Calculate jump addresses
    leal    (enter_protected_mode - SmpTrampolineStart), %eax
    addl    %ebx, %eax
    movl    %eax, (faraddr_protected_mode - SmpTrampolineStart)

    leal    (enter_long_mode - SmpTrampolineStart), %eax
    addl    %ebx, %eax
    movl    %eax, (faraddr_long_mode - SmpTrampolineStart)

    # Load IDT with no entries - forces triple fault on NMIs
    lidtl   IDT_PTR - SmpTrampolineStart

    # Load GDT with 32 bits descriptors
    lgdtl   GDT_PTR - SmpTrampolineStart

    # Jump to protected mode, but do not enable paging just yet.
    # The manual state that one should switch from protected mode
    # to long mode and not directly from real mode to protected mode.
    movl    $0x00000001, %eax       # PE
    movl    %eax, %cr0

    ljmpl   (faraddr_protected_mode - SmpTrampolineStart)


.code32

enter_protected_mode:

    mov     $0x10, %ax
    mov     %ax, %ds
    mov     %ax, %ss

    # Enable long mode + NX
    movl    $0xC0000080, %ecx
    rdmsr
    or      $0x9, %ah
    wrmsr

    # Setup paging
    movl    PARAM_CR3(%ebx), %eax   # eax = page table location
    movl    %eax, %cr3              # Set page table

    movl    $0x000000a0, %eax       # PGE + PAE
    movl    %eax, %cr4

    movl    $0x80000011, %eax       # PG + ET + PE
    movl    %eax, %cr0

    ljmpl   (faraddr_long_mode - SmpTrampolineStart)(%ebx)


faraddr_protected_mode:
    .long 0         # address
    .word 0x08      # selector

faraddr_long_mode:
    .long 0         # address
    .word 0x18      # selector


.code64

enter_long_mode:

    # Load 64 bits segments from new GDT
    movl    $0x20, %eax
    movl    %eax, %ds
    movl    %eax, %ss

    # Notify main processor that we reached the end of the trampoline code
    movl    $2, PARAM_FLAG(%ebx)

    # Setup stack
    movq    PARAM_STACK(%rbx), %rsp
    pushq $1

    # Jump to kernel
    movq    %rbx, %rdi
    addq    $PARAM_LOCATION, %rdi       # Param 1: TrampolineContext*
    call    PARAM_ENTRY(%ebx)

    # Hang on unexpected return
    cli
0:
    hlt
    jmp     0b


.align 16

GDT:
GDT_NULL:
    .quad   0

GDT_CODE_32: # 0x08
    .word   0xFFFF  # Limit = 0x100000 * 4 KB = 4 GB
    .word   0x0000  # Base = 0
    .word   0x9A00  # P + DPL 0 + S + Code + Execute + Read
    .word   0x00CF  # G + D (32 bits)

GDT_DATA_32: # 0x10
    .word   0xFFFF  # Limit = 0x100000 * 4 KB = 4 GB
    .word   0x0000  # Base = 0
    .word   0x9200  # P + DPL 0 + S + Data + Read + Write
    .word   0x00CF  # G + B (32 bits)

GDT_CODE_64: # 0x18
    .word   0
    .word   0
    .word   0x9A00  # P + DPL 0 + S + Code + Execute + Read
    .word   0x0020  # Long mode

GDT_DATA_64: # 0x20
    .word   0
    .word   0
    .word   0x9200  # P + DPL 0 + S + Data + Read + Write
    .word   0x0000  # Nothing

GDT_PTR:
    .word   GDT_PTR - GDT - 1
    .long   0
    .long   0

IDT_PTR:
    .word  0        # no entries
    .long  0        # base, don't care


SmpTrampolineEnd:
