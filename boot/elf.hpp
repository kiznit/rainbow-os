/*
    Copyright (c) 2016, Thierry Tremblay
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

#ifndef _RAINBOW_BOOT_ELF_HPP
#define _RAINBOW_BOOT_ELF_HPP

#include <stddef.h>
#include <rainbow/elf.h>



class Elf32Loader
{
public:

    Elf32Loader(const char* elfImage, size_t elfImageSize);

    // Is this a valid ELF file?
    bool Valid() const                      { return m_ehdr != NULL; }

    // Start / end address of ELF image
    uint32_t GetStartAddress() const        { return m_startAddress; }
    uint32_t GetEndAddress() const          { return m_endAddress; }

    // Return the memory size required to load this ELF
    uint32_t GetMemorySize() const          { return m_endAddress - m_startAddress; }

    // Return the memory alignment required to load this ELF
    uint32_t GetMemoryAlignment() const     { return m_alignment; }

    // Object File Type
    int GetType() const                     { return m_ehdr->e_type; }

    // Load the ELF file, return the entry point
    uint32_t Load(void* memory);


private:

    // Helpers
    const Elf32_Phdr* GetProgramHeader(int index) const;
    const Elf32_Shdr* GetSectionHeader(int index) const;

    bool LoadProgramHeaders(char* memory);
    bool ApplyRelocations(char* memory);

    const char*         m_image;        // Start of file in memory
    const size_t        m_imageSize;    // ELF file size
    const Elf32_Ehdr*   m_ehdr;         // ELF file header

    uint32_t            m_startAddress; // ELF start address
    uint32_t            m_endAddress;   // ELF end address
    uint32_t            m_alignment;    // ELF alignment
};



class Elf64Loader
{
public:

    Elf64Loader(const char* elfImage, size_t elfImageSize);

    // Is this a valid ELF file?
    bool Valid() const                      { return m_ehdr != NULL; }

    // Start / end address of ELF image
    uint64_t GetStartAddress() const        { return m_startAddress; }
    uint64_t GetEndAddress() const          { return m_endAddress; }

    // Return the memory size required to load this ELF
    uint64_t GetMemorySize() const          { return m_endAddress - m_startAddress; }

    // Return the memory alignment required to load this ELF
    uint64_t GetMemoryAlignment() const     { return m_alignment; }

    // Object File Type
    int GetType() const                     { return m_ehdr->e_type; }

    // Load the ELF file, return the entry point
    uint64_t Load(void* memory);


private:

    // Helpers
    const Elf64_Phdr* GetProgramHeader(int index) const;
    const Elf64_Shdr* GetSectionHeader(int index) const;

    bool LoadProgramHeaders(char* memory);
    bool ApplyRelocations(char* memory);

    const char*         m_image;        // Start of file in memory
    const size_t        m_imageSize;    // ELF file size
    const Elf64_Ehdr*   m_ehdr;         // ELF file header

    uint64_t            m_startAddress; // ELF start address
    uint64_t            m_endAddress;   // ELF end address
    uint64_t            m_alignment;    // ELF alignment
};



class ElfLoader
{
public:

    ElfLoader(const char* elfImage, size_t elfImageSize)
    :   m_elf32(elfImage, elfImageSize),
        m_elf64(elfImage, elfImageSize)
    {
    }

    // Is this a valid ELF file?
    bool Valid() const                  { return m_elf32.Valid() || m_elf64.Valid(); }
    bool Is32Bits() const               { return m_elf32.Valid(); }
    bool Is64Bits() const               { return m_elf64.Valid(); }

    // Start / end address of ELF image
    uint64_t GetStartAddress() const    { return m_elf32.Valid() ? m_elf32.GetStartAddress() : m_elf64.GetStartAddress(); }
    uint64_t GetEndAddress() const      { return m_elf32.Valid() ? m_elf32.GetEndAddress() : m_elf64.GetEndAddress(); }

    // Return the memory size required to load this ELF
    size_t GetMemorySize() const        { return m_elf32.Valid() ? m_elf32.GetMemorySize() : m_elf64.GetMemorySize(); }

    // Return the memory alignment required to load this ELF
    size_t GetMemoryAlignment() const   { return m_elf32.Valid() ? m_elf32.GetMemoryAlignment() : m_elf64.GetMemoryAlignment(); }

    // Object File Type
    int GetType() const                 { return m_elf32.Valid() ? m_elf32.GetType() : m_elf64.GetType(); }

    // Load the ELF file, return the entry point
    uint64_t Load(void* memory)         { return m_elf32.Valid() ? m_elf32.Load(memory) : m_elf64.Load(memory); }


private:

    Elf32Loader m_elf32;
    Elf64Loader m_elf64;
};



#endif
