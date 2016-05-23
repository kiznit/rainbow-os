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

%ifndef _RAINBOW_INCLUDED_KERNEL_X86_INC
%define _RAINBOW_INCLUDED_KERNEL_X86_INC


X86_CR0_PE equ (1 << 0)
X86_CR0_MP equ (1 << 1)
X86_CR0_EM equ (1 << 2)
X86_CR0_TS equ (1 << 3)
X86_CR0_ET equ (1 << 4)
X86_CR0_NE equ (1 << 5)
X86_CR0_WP equ (1 << 16)
X86_CR0_AM equ (1 << 18)
X86_CR0_NW equ (1 << 29)
X86_CR0_CD equ (1 << 30)
X86_CR0_PG equ (1 << 31)


X86_CR4_OSFXSR      equ (1 << 9)
X86_CR4_OSXMMEXCPT  equ (1 << 10)


%endif
