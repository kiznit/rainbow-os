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
#   Multiboot headers
#
#   Multiboot 2 header comes first in case some boot loader decides to scan
#   for both multiboot signatures simultaneously. We want multiboot 2 to take
#   precedence over multiboot 1.
#
###############################################################################

.section .multiboot
.code32

    jmp _start                  # In case someone jumps to the start of the boot image
    .asciz "RAINBOW_MULTIBOOT"  # Signature


.equ MULTIBOOT_HEADER_MAGIC          , 0x1BADB002
.equ MULTIBOOT_HEADER_FLAGS          , 0x00000007   # VIDEO_MODE (4) + MEMORY_INFO (2) + PAGE_ALIGN (1)
.equ MULTIBOOT_HEADER_CHECKSUM       , -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS) & 0xFFFFFFFF

.equ MULTIBOOT2_HEADER_MAGIC         , 0xe85250d6
.equ MULTIBOOT2_HEADER_ARCHITECTURE  , 0
.equ MULTIBOOT2_HEADER_LENGTH        , multiboot2_header_end - multiboot2_header
.equ MULTIBOOT2_HEADER_CHECKSUM      , -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT2_HEADER_ARCHITECTURE + MULTIBOOT2_HEADER_LENGTH) & 0xFFFFFFFF


.align 16, 0

# Fixed header
multiboot2_header:
    .long   MULTIBOOT2_HEADER_MAGIC
    .long   MULTIBOOT2_HEADER_ARCHITECTURE
    .long   MULTIBOOT2_HEADER_LENGTH
    .long   MULTIBOOT2_HEADER_CHECKSUM

# Define entry point
.align 8, 0
    .word   3       # entry address tag
    .word   1       # flags = optional
    .long   12      # size of tag
    .long   _start  # entry_addr

# Video mode
.align 8, 0
    .word   5       # framebuffer tag
    .word   0       # flags
    .long   20      # size of tag
    .long   0       # Preferred width
    .long   0       # Preferred height
    .long   32      # Preferred pixel depth

# Module alignment
.align 8, 0
    .word   6       # module alignment tag
    .word   0       # flags
    .long   8       # size of tag

# End of tag list
.align 8, 0
    .word   0       # end tag
    .word   0       # flags
    .long   8       # size of tag

multiboot2_header_end:


.align 16, 0
multiboot_header:
    .long   MULTIBOOT_HEADER_MAGIC
    .long   MULTIBOOT_HEADER_FLAGS
    .long   MULTIBOOT_HEADER_CHECKSUM

    # aout kludge (unused)
    .long 0,0,0,0,0

    # Video mode
    .long   0       # Linear graphics please?
    .long   0       # Preferred width
    .long   0       # Preferred height
    .long   32      # Preferred pixel depth


###############################################################################
#
# Program entry point
#
###############################################################################

.section .text
.code32

.global _start
_start:
    cli                     # Disable interrupts
    cld                     # Clear direction flag
    movl    $__stack_end, %esp  # Initialize stack

    # These will be passed as parameters to multiboot_main()
    pushl   %ebx            # multiboot_header*
    pushl   %eax            # MULTIBOOT_BOOTLOADER_MAGIC

    # Initialize the GDT & reload segment descriptors
    lgdt    GDT_PTR

    # Load 32 bits segments from new GDT
    ljmpl   $0x18, $1f
1:
    movl    $0x20, %eax
    movl    %eax, %ds
    movl    %eax, %es
    movl    %eax, %fs
    movl    %eax, %gs
    movl    %eax, %ss

    call    _init           # Call global constructors

    call    multiboot_main  # Execute bootloader, not expected to return
    addl    $8, %esp        # Remove arguments to multiboot_main()

    call    _fini           # Shutdown

.halt:
    cli                     # Disable interrupts
    hlt                     # Halt the CPU
    jmp .halt               # NMI can wake up CPU, go back to sleep


###############################################################################
#
# Trampoline to jump to the kernel
#
###############################################################################

#if defined(KERNEL_IA32)

.global jumpToKernel
jumpToKernel:
    # Enable PAE and PGE
    movl    %cr4, %eax
    orl     $0xa0, %eax
    movl    %eax, %cr4

    # Set page tables
    movl    16(%esp), %eax  # eax = page tables
    movl    %eax, %cr3

    # Enable paging
    movl    %cr0, %eax
    btsl    $31, %eax
    movl    %eax, %cr0

    # Jump to kernel
    movl    4(%esp), %eax   # eax = kernel entry point
    movl    12(%esp), %ecx  # ecx = BootInfo*

    pushl   %ecx            # BootInfo* parameter for kernel
    call    *%eax           # Call kernel

    jmp     .halt;

#endif


###############################################################################
#
# Trampoline from 32 bits bootloader to 64 bits kernel
#
###############################################################################

#if defined(KERNEL_X86_64)

.global jumpToKernel
jumpToKernel:
    # Enable PAE and PGE
    movl    %cr4, %eax
    orl     $0xa0, %eax
    movl    %eax, %cr4

    # Enable long mode
    movl    $0xC0000080, %ecx
    rdmsr
    btsl    $8, %eax
    wrmsr

    # Set page tables
    movl    16(%esp), %eax  # eax = page tables
    movl    %eax, %cr3

    # Enable paging
    movl    %cr0, %eax
    btsl    $31, %eax
    movl    %eax, %cr0

    # Load 64-bits segments from GDT
    movl    $0x30, %eax
    movl    %eax, %ds
    movl    %eax, %ss

    # Far jump into long mode. Note that it is impossible to jump
    # directly to a 64 bits address.
    ljmp    $0x28, $enter_long_mode

.code64

enter_long_mode:
    # Jump to kernel
    movq    4(%rsp), %rcx   # rcx = kernel entry point
    movl    12(%rsp), %edi  # rdi = BootInfo* parameter for kernel
    call    *%rcx           # Call kernel

cpu_halt:
    cli
    hlt
    jmp cpu_halt

#endif



.align 16

GDT:
GDT_NULL:
    .quad   0

GDT_CODE_16: # 0x08
    .word   0xFFFF      # Limit = 0x100000 = 1 MB
    .word   0x0000      # Base = 0
    .word   0x9A00      # P + DPL 0 + S + Code + Execute + Read
    .word   0x000F      # Limit (top 4 bits)

GDT_DATA_16: # 0x10
    .word   0xFFFF      # Limit = 0x100000 = 1 MB
    .word   0x0000      # Base = 0
    .word   0x9200      # P + DPL 0 + S + Data + Read + Write
    .word   0x000F      # Limit (top 4 bits)

GDT_CODE_32: # 0x18
    .word   0xFFFF      # Limit = 0x100000 * 4 KB = 4 GB
    .word   0x0000      # Base = 0
    .word   0x9A00      # P + DPL 0 + S + Code + Execute + Read
    .word   0x00CF      # G + D (32 bits)

GDT_DATA_32: # 0x20
    .word   0xFFFF      # Limit = 0x100000 * 4 KB = 4 GB
    .word   0x0000      # Base = 0
    .word   0x9200      # P + DPL 0 + S + Data + Read + Write
    .word   0x00CF      # G + B (32 bits)

#if defined(KERNEL_X86_64)

GDT_CODE_64: # 0x28
    .word   0
    .word   0
    .word   0x9A00      # P + DPL 0 + S + Code + Execute + Read
    .word   0x0020      # Long mode

GDT_DATA_64: # 0x30
    .word   0
    .word   0
    .word   0x9200      # P + DPL 0 + S + Data + Read + Write
    .word   0x0000      # Nothing

#endif

GDT_PTR:
    .word   GDT_PTR - GDT - 1
    .long   GDT
    .long   0
