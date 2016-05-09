; Copyright (c) 2016, Thierry Tremblay
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


global StartKernel32
global StartKernel64


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Enable paging and jump to the kernel (32 bits)
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

bits 32
section .text

; void StartKernel32(BootInfo* bootInfo, uint32_t cr3, uint32_t entry);
;
;   [esp]       return address
;   [esp+4]     bootInfo
;   [esp+8]     cr3
;   [esp+12]    entry

StartKernel32:

    ; Enable PAE
    mov eax, cr4
    bts eax, 5
    mov cr4, eax

    ; Set CR3
    mov eax, [esp+8]
    mov cr3, eax

    ; Enable paging
    mov eax, cr0
    bts eax, 31
    mov cr0, eax

    ; Jump to kernel using an absolute jump
    mov ecx, [esp+12]
    jmp ecx




;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Enable paging and jump to the kernel (64 bits)
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

bits 32
section .text

; void StartKernel64(BootInfo* bootInfo, uint32_t cr3, uint64_t entry);
;
;   [esp]       return address
;   [esp+4]     bootInfo
;   [esp+8]     cr3
;   [esp+12]    entry

StartKernel64:

    ; Enable PAE
    mov eax, cr4
    bts eax, 5
    mov cr4, eax

    ; Set CR3
    mov eax, [esp+8]
    mov cr3, eax

    ; Enable long mode
    mov ecx, 0xC0000080
    rdmsr
    bts eax, 8
    wrmsr

    ; Enable paging
    mov eax, cr0
    bts eax, 31
    mov cr0, eax

    ; Enable SSE
    mov eax, cr0
    bts eax, 1
    btr eax, 2
    mov cr0, eax
    mov eax, cr4
    or eax, 0x600
    mov cr4, eax

    ; Load temporary GDT
    lgdt [GDTR]

    ; Load segments
    mov eax, GDT_BOOT_DATA
    mov ds, eax
    mov es, eax
    mov ss, eax

    ; Far jump into long mode. Note that it is impossible to do an absolute jump
    ; to a 64-bit address from a 32 bits code segment. So we will jump to a 32 bits
    ; address first and then jump to the kernel.
    jmp GDT_BOOT_CODE:enter_long_mode


enter_long_mode:

    ; BootInfo* parameter needs to be passed in rdi
    mov edi, [esp+4]

    ; Jump to kernel using an absolute jump
    ; mov rcx, [esp+12]
    ; jmp rcx

    ; nasm won't let us use 64 bits instruction in 32 bits,
    ; so we enter the instructions using db.

    db 0x67, 0x48, 0x8b, 0x4c, 0x24, 0x0c   ; mov    rcx, [esp+12]
    db 0xff, 0xe1                           ; jmp    rcx




section .rodata
align 16

GDT_BOOT_NULL equ gdt_boot_null - GDT
GDT_BOOT_CODE equ gdt_boot_code - GDT
GDT_BOOT_DATA equ gdt_boot_data - GDT

GDT:
gdt_boot_null:
    dq 0

gdt_boot_code:
    dw 0
    dw 0
    db 0
    db 10011010b    ; P + DPL 0 + S + Code + Execute + Read
    db 00100000b    ; Long mode
    db 0

gdt_boot_data:
    dw 0
    dw 0
    db 0
    db 10010010b    ; P + DPL 0 + S + Data + Read + Write
    db 00000000b
    db 0

GDTR:
    dw GDTR - GDT - 1
    dd GDT
    dd 0
