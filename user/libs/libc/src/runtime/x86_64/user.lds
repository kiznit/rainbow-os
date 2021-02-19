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


OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)


PHDRS
{
    phdr_phdrs  PT_PHDR PHDRS;
    phdr_text   PT_LOAD PHDRS FLAGS(5);     /* read + execute */
    phdr_rodata PT_LOAD FLAGS(4);           /* read */
    phdr_data   PT_LOAD FLAGS(6);           /* read + write */
    phdr_tls    PT_TLS  FLAGS(4);
}


SECTIONS
{
    . = 0x00010000 + SIZEOF_HEADERS;

    .text :
    {
        *(.text*)
    } :phdr_text

    . = ALIGN(4K);

    .rodata :
    {
        *(.rodata*)
    } :phdr_rodata

    . = ALIGN(4K);

    .data :
    {
        *(.data*)
    } :phdr_data

    .bss :
    {
        *(.bss)
    } :phdr_data

    . = ALIGN(4K);

    .tdata :
    {
        *(.tdata*)
     } :phdr_tls :phdr_data

    .tbss :
    {
        *(.tbss)
    } :phdr_tls
}
