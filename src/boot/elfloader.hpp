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


class Elf32Loader
{
public:

    Elf32Loader(const void* elfImage, size_t elfImageSize);

    // Is this a valid ELF file?
    bool Valid() const                      { return m_ehdr != NULL; }

    // Target machine
    int GetMachine() const                  { return m_ehdr->e_machine; }

    // Object File Type
    int GetType() const                     { return m_ehdr->e_type; }

    // Load the ELF file, return the entry point
    uint32_t Load();


private:

    // Helpers
    const Elf32_Phdr* GetProgramHeader(int index) const;

    bool LoadProgramHeaders();

    const char*         m_image;        // Start of file in memory
    const Elf32_Ehdr*   m_ehdr;         // ELF file header
};



class Elf64Loader
{
public:

    Elf64Loader(const void* elfImage, size_t elfImageSize);

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
    const Elf64_Phdr* GetProgramHeader(int index) const;

    bool LoadProgramHeaders();

    const char*         m_image;        // Start of file in memory
    const Elf64_Ehdr*   m_ehdr;         // ELF file header
};



class ElfLoader
{
public:

    ElfLoader(const void* elfImage, size_t elfImageSize)
    :   m_elf32(elfImage, elfImageSize),
        m_elf64(elfImage, elfImageSize)
    {
    }

    // Is this a valid ELF file?
    bool Valid() const                  { return m_elf32.Valid() || m_elf64.Valid(); }
    bool Is32Bits() const               { return m_elf32.Valid(); }
    bool Is64Bits() const               { return m_elf64.Valid(); }

    // Target machine
    int GetMachine() const              { return m_elf32.Valid() ? m_elf32.GetMachine() : m_elf64.GetMachine(); }

    // Object File Type
    int GetType() const                 { return m_elf32.Valid() ? m_elf32.GetType() : m_elf64.GetType(); }

    // Load the ELF file, return the entry point
    uint64_t Load()                     { return m_elf32.Valid() ? m_elf32.Load() : m_elf64.Load(); }


private:

    Elf32Loader m_elf32;
    Elf64Loader m_elf64;
};



#endif
