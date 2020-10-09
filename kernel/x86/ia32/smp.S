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
.equ PARAM_ENTRY,       PARAM_LOCATION + 0x0C
.equ PARAM_CPU,         PARAM_LOCATION + 0x10


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

    # Calculate jump address
    leal    (enter_protected_mode - SmpTrampolineStart), %eax
    addl    %ebx, %eax
    movl    %eax, (faraddr - SmpTrampolineStart)

    # Load IDT with no entries - forces triple fault on NMIs
    lidtl   IDT_PTR - SmpTrampolineStart

    # Load GDT with 32 bits descriptors
    lgdtl   GDT_PTR - SmpTrampolineStart

    # Enable NX
    movl    $0xC0000080, %ecx
    rdmsr
    or      $0x8, %ah
    wrmsr

    # Setup paging
    movl    PARAM_CR3, %eax         # eax = page table location
    movl    %eax, %cr3              # Set page table

    movl    $0x000000a0, %eax       # PGE + PAE
    movl    %eax, %cr4

    movl    $0x80000011, %eax       # PG + ET + PE
    movl    %eax, %cr0

    ljmpl   (faraddr - SmpTrampolineStart)


faraddr:
    .long 0         # address
    .word 0x08      # selector


.code32

enter_protected_mode:

    # Load 32 bits segments from new GDT
    movl    $0x10, %eax
    movl    %eax, %ds
    movl    %eax, %es
    movl    %eax, %fs
    movl    %eax, %gs
    movl    %eax, %ss

    # Notify main processor that we reached the end of the trampoline code
    movl    $2, PARAM_FLAG(%ebx)

    # Setup stack
    movl    PARAM_STACK(%ebx), %esp

    # Jump to kernel
    movl    %ebx, %eax
    addl    $PARAM_LOCATION, %eax   # eax = TrampolineContext*
    pushl   %eax                    # Param 1: TrampolineContext*
    pushl   $cpu_halt               # Return to cpu_halt()
    jmp     PARAM_ENTRY(%ebx)       # Fake call to smp entry point


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

GDT_PTR:
    .word   GDT_PTR - GDT - 1
    .long   0
    .long   0

IDT_PTR:
    .word  0        # no entries
    .long  0        # base, don't care


SmpTrampolineEnd:
