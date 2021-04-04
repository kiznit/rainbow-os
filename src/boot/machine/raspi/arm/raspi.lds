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


OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)
ENTRY(_start)


PHDRS
{
    phdr_text   PT_LOAD FLAGS(5);   /* read + execute */
    phdr_rodata PT_LOAD FLAGS(4);   /* read */
    phdr_data   PT_LOAD FLAGS(6);   /* read + write */
}


SECTIONS
{
    . = 1M;

    ImageBase = .;

    .text ALIGN(4K) :
    {
        *(.text*)
    } :phdr_text

    .rodata ALIGN(4K) :
    {
        *(.rodata*)
    } :phdr_rodata

    .ARM.exidx :
    {
        *(.ARM.exidx*)
    }

    .data ALIGN(4K) :
    {
        *(.got.plt)
        *(.got)
        *(.data*)
    } :phdr_data

    .init_array ALIGN(4):
    {
        __init_array_start = .;
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        __init_array_end = .;
    }

    .fini_array ALIGN(4):
    {
        __fini_array_start = .;
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        __fini_array_end = .;
    }

    .bss :
    {
        *(.bss)
        *(COMMON)
    } :phdr_data

    .heap ALIGN(4K) :
    {
        __heap_start = .;
        . = . + 128K;
        __heap_end = .;
    }

    .stack ALIGN(4K) :
    {
        __stack_start = .;
        . = . + 128K;
        __stack_end = .;
    }

    /DISCARD/ :
    {
        *(.comment)
    }

    ImageEnd = .;
}
