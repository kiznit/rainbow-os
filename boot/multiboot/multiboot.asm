; Copyright (c) 2015, Thierry Tremblay
; All rights reserved.
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions are met:
;
; * Redistributions of source code must retain the above copyright notice, this
;   list of conditions and the following disclaimer.
;
; * Redistributions in binary form must reproduce the above copyright notice,
;   this list of conditions and the following disclaimer in the documentation
;   and/or other materials provided with the distribution.
;
; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
; AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
; IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
; DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
; FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
; DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
; SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
; CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
; OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
; OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

bits 32


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Multiboot headers
;
; Multiboot 2 header comes first in case some boot loader decides to scan
; for both multiboot signatures simultaneously. We want multiboot 2 to take
; precedence over multiboot 1.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

section .multiboot

    jmp _start                  ; In case someone jumps to the start of the boot image
    db "RAINBOW_MULTIBOOT", 0   ; Signature


MULTIBOOT_HEADER_MAGIC          equ 0x1BADB002
MULTIBOOT_HEADER_FLAGS          equ 0x00000007
MULTIBOOT_HEADER_CHECKSUM       equ -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS) & 0xFFFFFFFF

MULTIBOOT2_HEADER_MAGIC         equ 0xe85250d6
MULTIBOOT2_HEADER_ARCHITECTURE  equ 0
MULTIBOOT2_HEADER_LENGTH        equ multiboot2_header_end - multiboot2_header
MULTIBOOT2_HEADER_CHECKSUM      equ -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT2_HEADER_ARCHITECTURE + MULTIBOOT2_HEADER_LENGTH) & 0xFFFFFFFF


align 16, db 0
multiboot2_header:
    dd  MULTIBOOT2_HEADER_MAGIC
    dd  MULTIBOOT2_HEADER_ARCHITECTURE
    dd  MULTIBOOT2_HEADER_LENGTH
    dd  MULTIBOOT2_HEADER_CHECKSUM

align 8, db 0
    dw  3       ; entry address tag
    dw  1       ; flags = optional
    dd  12      ; size of tag
    dd  _start  ; entry_addr

;align 8, db 0
;    dw  5       ; framebuffer tag
;    dw  0       ; flags
;    dd  20      ; size of tag
;    dd  0       ; Preferred width
;    dd  0       ; Preferred height
;    dd  32      ; Preferred pixel depth

align 8, db 0
    dw  0       ; end tag
    dw  0       ; flags
    dd  8       ; size of tag

multiboot2_header_end:



align 16, db 0
multiboot_header:
    dd  MULTIBOOT_HEADER_MAGIC
    dd  MULTIBOOT_HEADER_FLAGS
    dd  MULTIBOOT_HEADER_CHECKSUM

    ; aout kludge (unused)
    dd 0,0,0,0,0

    ; Video mode
    dd  1           ; Linear graphics please?
    dd  0           ; Preferred width
    dd  0           ; Preferred height
    dd  32          ; Preferred pixel depth



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Entry point
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

section .text

global _start
extern multiboot_main

_start:
    cli                         ; Disable interrupts
    cld                         ; Clear direction flag
    mov esp, _boot_stack_top    ; Initialize stack

    push ebx                    ; multiboot_header*
    push eax                    ; MULTIBOOT_BOOTLOADER_MAGIC
    call multiboot_main
    add  esp, 8                 ; Pop arguments to multiboot_main()

.halt:
    cli                         ; Disable interrupts
    hlt                         ; Halt the CPU
    jmp .halt                   ; NMI can wake up CPU, go back to sleep



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Boot Stack
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

section .bss nobits
align 0x1000

_boot_stack_base:
    resb 0x10000
_boot_stack_top:
