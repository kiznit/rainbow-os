/*
    Copyright (c) 2018, Thierry Tremblay
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


OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)


PHDRS
{
    phdr_text PT_LOAD;
    phdr_rodata PT_LOAD;
    phdr_data PT_LOAD;
}


SECTIONS
{
    . = 0xFFFFFFFFF0000000;

    .text ALIGN(4K) :
    {
        *(.text*)
    } :phdr_text

    .rodata ALIGN(4K) :
    {
        *(.rodata*)
    } :phdr_rodata

    .data ALIGN(4K) :
    {
        *(.data*)
    } :phdr_data

    .init_array :
    {
        . = ALIGN(8);
        PROVIDE_HIDDEN(__init_array_start = .);
        *(SORT(.init_array.*))
        *(.init_array)
        PROVIDE_HIDDEN(__init_array_end = .);
    }

    .bss ALIGN(4K) :
    {
        *(.bss)
    } :phdr_data

    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame)
    }
}
