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
