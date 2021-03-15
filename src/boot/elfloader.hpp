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

#ifndef _RAINBOW_BOOT_ELFLOADER_HPP
#define _RAINBOW_BOOT_ELFLOADER_HPP

#include <stddef.h>
#include <elf.h>


class ElfLoader
{
public:

#if defined(KERNEL_IA32)
    typedef Elf32_Phdr Elf_Phdr;
    typedef Elf32_Ehdr Elf_Ehdr;
    constexpr static uint8_t  ELF_CLASS   = ELFCLASS32;
    constexpr static uint16_t ELF_MACHINE = EM_386;
#elif defined(KERNEL_X86_64)
    typedef Elf64_Phdr Elf_Phdr;
    typedef Elf64_Ehdr Elf_Ehdr;
    constexpr static uint8_t  ELF_CLASS   = ELFCLASS64;
    constexpr static uint16_t ELF_MACHINE = EM_X86_64;
#endif

    ElfLoader(const void* elfImage, size_t elfImageSize);

    // Is this a valid ELF file?
    bool Valid() const                      { return m_ehdr != NULL; }

    // Target machine
    int GetMachine() const                  { return m_ehdr->e_machine; }

    // Object File Type
    int GetType() const                     { return m_ehdr->e_type; }

    // Load the ELF file, return the entry point
    uint64_t Load();


private:

    // Helpers
    const Elf_Phdr* GetProgramHeader(int index) const;

    bool LoadProgramHeaders();

    const char*         m_image;        // Start of file in memory
    const Elf_Ehdr*     m_ehdr;         // ELF file header
};


#endif
