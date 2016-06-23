/*
    Copyright (c) 2016, Thierry Tremblay
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


/******************************************************************************

    Multiboot headers

    Multiboot 2 header comes first in case some boot loader decides to scan
    for both multiboot signatures simultaneously. We want multiboot 2 to take
    precedence over multiboot 1.

******************************************************************************/

.section .multiboot
.code32

    jmp _start                  // In case someone jumps to the start of the boot image
    .asciz "RAINBOW_MULTIBOOT"  // Signature


.equ MULTIBOOT_HEADER_MAGIC          , 0x1BADB002
.equ MULTIBOOT_HEADER_FLAGS          , 0x00000007
.equ MULTIBOOT_HEADER_CHECKSUM       , -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS) & 0xFFFFFFFF

.equ MULTIBOOT2_HEADER_MAGIC         , 0xe85250d6
.equ MULTIBOOT2_HEADER_ARCHITECTURE  , 0
.equ MULTIBOOT2_HEADER_LENGTH        , multiboot2_header_end - multiboot2_header
.equ MULTIBOOT2_HEADER_CHECKSUM      , -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT2_HEADER_ARCHITECTURE + MULTIBOOT2_HEADER_LENGTH) & 0xFFFFFFFF


.align 16, 0

multiboot2_header:
    .long   MULTIBOOT2_HEADER_MAGIC
    .long   MULTIBOOT2_HEADER_ARCHITECTURE
    .long   MULTIBOOT2_HEADER_LENGTH
    .long   MULTIBOOT2_HEADER_CHECKSUM

.align 8, 0
    .word   3       // entry address tag
    .word   1       // flags = optional
    .long   12      // size of tag
    .long   _start  // entry_addr

/*
.align 8, 0
    .word   5       // framebuffer tag
    .word   0       // flags
    .long   20      // size of tag
    .long   0       // Preferred width
    .long   0       // Preferred height
    .long   32      // Preferred pixel depth
*/

.align 8, 0
    .word   0       // end tag
    .word   0       // flags
    .long   8       // size of tag

multiboot2_header_end:


.align 16, 0
multiboot_header:
    .long   MULTIBOOT_HEADER_MAGIC
    .long   MULTIBOOT_HEADER_FLAGS
    .long   MULTIBOOT_HEADER_CHECKSUM

    // aout kludge (unused)
    .long 0,0,0,0,0

    // Video mode
    .long   1       // Linear graphics please?
    .long   0       // Preferred width
    .long   0       // Preferred height
    .long   32      // Preferred pixel depth



/*
    Entry point
*/

.section .text
.code32

.globl _start

_start:
    cli                         // Disable interrupts
    cld                         // Clear direction flag
    movl $_boot_stack_top, %esp // Initialize stack

    pushl %ebx                  // multiboot_header*
    pushl %eax                  // MULTIBOOT_BOOTLOADER_MAGIC
    call multiboot_main         // Execute bootloader, not expected to return
    addl $8, %esp               // Pop arguments to multiboot_main()

.halt:
    cli                         // Disable interrupts
    hlt                         // Halt the CPU
    jmp .halt                   // NMI can wake up CPU, go back to sleep



/*
    Boot Stack
*/

.section .bss
.align 4096

_boot_stack:
.skip 65536
_boot_stack_top:
