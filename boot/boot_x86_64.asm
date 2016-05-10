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

bits 64
section .text

; void StartKernel32(BootInfo* bootInfo, uint32_t cr3, uint32_t entry);

StartKernel32:

    ret




;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Enable paging and jump to the kernel (64 bits)
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

bits 64
section .text

; void StartKernel64(BootInfo* bootInfo, uint32_t cr3, uint64_t entry);

StartKernel64:

    ret




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
    dq GDT
