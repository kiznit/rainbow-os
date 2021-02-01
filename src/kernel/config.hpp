/*
    Copyright (c) 2020, Thierry Tremblay
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

#ifndef _RAINBOW_KERNEL_CONFIG_HPP
#define _RAINBOW_KERNEL_CONFIG_HPP

#if defined(__i386__)

extern char _heap_start[];

static const int STACK_PAGE_COUNT = 1;

// TODO: on ia32, we mapped the framebuffer to 0xE0000000 in the bootloader
// The reason for this is that we have to ensure the framebuffer isn't in
// kernel space (>= 0xF0000000). This should go away once we more console
// rendering out of the kernel.
static void* const VMA_FRAMEBUFFER_START    = (void*)0xE0000000;
static void* const VMA_FRAMEBUFFER_END      = (void*)0xEFEFF000;

static void* const VMA_USER_STACK_START     = (void*)0xEFEFF000; // 1 MB
static void* const VMA_USER_STACK_END       = (void*)0xEFFFF000;

static void* const VMA_VDSO_START           = (void*)0xEFFFF000;
static void* const VMA_VDSO_END             = (void*)0xF0000000;

static void* const VMA_KERNEL_START         = (void*)0xF0000000;
static void* const VMA_KERNEL_END           = &_heap_start;

static void* const VMA_HEAP_START           = &_heap_start;
static void* const VMA_HEAP_END             = (void*)0xFF7FF000;
static void* const VMA_PAGE_TABLES_START    = (void*)0xFF7FF000;
static void* const VMA_PAGE_TABLES_END      = (void*)0xFFFFFFFF;

#elif defined(__x86_64__)

static const int STACK_PAGE_COUNT = 2;

static void* const VMA_USER_STACK_START     = (void*)0x00007FFFFFEFF000ull; // 1 MB
static void* const VMA_USER_STACK_END       = (void*)0x00007FFFFFFFF000ull;

static void* const VMA_VDSO_START           = (void*)0x00007FFFFFFFF000ull;
static void* const VMA_VDSO_END             = (void*)0x0000800000000000ull;

// TODO: on x86_64, we mapped the framebuffer to 0xFFFF800000000000 in the bootloader
// The reason for this is that we have to ensure the framebuffer isn't in
// user space. This should go away once we more console rendering out of the kernel.
static void* const VMA_FRAMEBUFFER_START    = (void*)0xFFFF800000000000ull;
static void* const VMA_FRAMEBUFFER_END      = (void*)0xFFFFFEFFFFFFFFFFull;

static void* const VMA_PAGE_TABLES_START    = (void*)0xFFFFFF0000000000ull;
static void* const VMA_PAGE_TABLES_END      = (void*)0xFFFFFF7FFFFFFFFFull;
static void* const VMA_HEAP_START           = (void*)0xFFFFFF8000000000ull;
static void* const VMA_HEAP_END             = (void*)0xFFFFFFFF80000000ull;

static void* const VMA_KERNEL_START         = (void*)0xFFFFFFFF80000000ull;
static void* const VMA_KERNEL_END           = (void*)0xFFFFFFFFFFFFFFFFull;

#else

#error Configuration not defined for this architecture.

#endif


#endif
