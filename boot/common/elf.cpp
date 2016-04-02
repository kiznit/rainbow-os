/*
    Copyright (c) 2015, Thierry Tremblay
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

#include "elf.hpp"
#include <stdio.h>
#include <string.h>



Elf32Loader::Elf32Loader(const char* elfImage, size_t elfImageSize)
:   m_image(elfImage),
    m_imageSize(elfImageSize),
    m_ehdr(NULL),
    m_startAddress(0),
    m_endAddress(0),
    m_alignment(0)
{
    if (elfImageSize < sizeof(Elf32_Ehdr))
    {
        return;
    }

    const Elf32_Ehdr* ehdr = (const Elf32_Ehdr*)elfImage;

    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
        ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
        ehdr->e_ident[EI_MAG3] != ELFMAG3 ||
        ehdr->e_ident[EI_DATA] != ELFDATA2LSB)
    {
        return;
    }

    if (ehdr->e_ident[EI_CLASS] != ELFCLASS32 ||
        ehdr->e_machine != EM_386 ||
        ehdr->e_version != EV_CURRENT)
    {
        return;
    }

    // ELF looks valid...
    m_ehdr = ehdr;


    // Calculate how much memory we need to load this ELF
    uint32_t start = 0xFFFFFFFF;
    uint32_t end = 0;
    uint32_t align = 1;

    for (int i = 0; i != m_ehdr->e_phnum; ++i)
    {
        const Elf32_Phdr* phdr = GetProgramHeader(i);

        if (phdr->p_type != PT_LOAD)
            continue;

        if (phdr->p_paddr < start)
            start = phdr->p_paddr;

        if (phdr->p_paddr + phdr->p_memsz > end)
            end = phdr->p_paddr + phdr->p_memsz;

        if (phdr->p_align > align)
            align = phdr->p_align;
    }

    m_startAddress = start;
    m_endAddress = end;
    m_alignment = align;
}



const Elf32_Phdr* Elf32Loader::GetProgramHeader(int index) const
{
    const char* phdr_base = m_image + m_ehdr->e_phoff;
    const Elf32_Phdr* phdr = (const Elf32_Phdr*)(phdr_base + index * m_ehdr->e_phentsize);
    return phdr;
}



const Elf32_Shdr* Elf32Loader::GetSectionHeader(int index) const
{
    const char* shdr_base = m_image + m_ehdr->e_shoff;
    const Elf32_Shdr* shdr = (const Elf32_Shdr*)(shdr_base + index * m_ehdr->e_shentsize);
    return shdr;
}



void* Elf32Loader::Load(char* memory)
{
    LoadProgramHeaders(memory);
    ApplyRelocations(memory);

    const uint32_t memoryOffset = (uint32_t)(uintptr_t)memory - m_startAddress;
    void* entry = (void*)(uintptr_t)(m_ehdr->e_entry + memoryOffset);
    return entry;
}



void Elf32Loader::LoadProgramHeaders(char* memory)
{
    for (int i = 0; i != m_ehdr->e_phnum; ++i)
    {
        const Elf32_Phdr* phdr = GetProgramHeader(i);

        if (phdr->p_type != PT_LOAD)
            continue;

        if (phdr->p_filesz != 0)
        {
            const char* src = m_image + phdr->p_offset;
            void* dst = memory + (phdr->p_paddr - m_startAddress);
            memcpy(dst, src, phdr->p_filesz);
        }

        if (phdr->p_memsz > phdr->p_filesz)
        {
            void* dst = memory + (phdr->p_paddr - m_startAddress) + phdr->p_filesz;
            const size_t count = phdr->p_memsz - phdr->p_filesz;
            memset(dst, 0, count);
        }
    }
}



void Elf32Loader::ApplyRelocations(char* memory)
{
    const uint32_t memoryOffset = (uint32_t)(uintptr_t)memory - m_startAddress;

    for (int i = 0; i != m_ehdr->e_shnum; ++i)
    {
        const Elf32_Shdr* shdr = GetSectionHeader(i);

        if (shdr->sh_type != SHT_REL)
            continue;

        const Elf32_Shdr* symbols_section = GetSectionHeader(shdr->sh_link);
        const char* symbols_base = m_image + symbols_section->sh_offset;

        const char* rel_base = m_image + shdr->sh_offset;

        for (int j = 0; j != (int)(shdr->sh_size / shdr->sh_entsize); ++j)
        {
            const Elf32_Rel* rel = (const Elf32_Rel*)(rel_base + j * shdr->sh_entsize);

            const int sym = ELF32_R_SYM(rel->r_info);
            const int type = ELF32_R_TYPE(rel->r_info);

            const Elf32_Sym* symbol = (const Elf32_Sym*)(symbols_base + sym * symbols_section->sh_entsize);

            switch (type)
            {
            case R_386_32:
                // Symbol value + addend
                *(uint32_t*)(memory + rel->r_offset - m_startAddress) += symbol->st_value + memoryOffset;
                break;

            case R_386_GLOB_DAT:
                // Symbol value
                *(uint32_t*)(memory + rel->r_offset - m_startAddress) = symbol->st_value + memoryOffset;
                break;

            case R_386_RELATIVE:
                // Base address + addend
                *(uint32_t*)(memory + rel->r_offset - m_startAddress) += memoryOffset;
                break;

            default:
                printf("Elf32Loader: unknown relocation type %d!\n", type);
                break;
            }
        }
    }
}



Elf64Loader::Elf64Loader(const char* elfImage, size_t elfImageSize)
:   m_image(elfImage),
    m_imageSize(elfImageSize),
    m_ehdr(NULL),
    m_startAddress(0),
    m_endAddress(0),
    m_alignment(0)
{
    if (elfImageSize < sizeof(Elf64_Ehdr))
    {
        return;
    }

    const Elf64_Ehdr* ehdr = (const Elf64_Ehdr*)elfImage;

    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
        ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
        ehdr->e_ident[EI_MAG3] != ELFMAG3 ||
        ehdr->e_ident[EI_DATA] != ELFDATA2LSB)
    {
        return;
    }

    if (ehdr->e_ident[EI_CLASS] != ELFCLASS64 ||
        ehdr->e_machine != EM_X86_64 ||
        ehdr->e_version != EV_CURRENT)
    {
        return;
    }

    // ELF looks valid...
    m_ehdr = ehdr;


    // Calculate how much memory we need to load this ELF
    uint64_t start = 0xFFFFFFFFFFFFFFFF;
    uint64_t end = 0;
    uint64_t align = 1;

    for (int i = 0; i != m_ehdr->e_phnum; ++i)
    {
        const Elf64_Phdr* phdr = GetProgramHeader(i);

        if (phdr->p_type != PT_LOAD)
            continue;

        if (phdr->p_paddr < start)
            start = phdr->p_paddr;

        if (phdr->p_paddr + phdr->p_memsz > end)
            end = phdr->p_paddr + phdr->p_memsz;

        if (phdr->p_align > align)
            align = phdr->p_align;
    }

    m_startAddress = start;
    m_endAddress = end;
    m_alignment = align;
}



const Elf64_Phdr* Elf64Loader::GetProgramHeader(int index) const
{
    const char* phdr_base = m_image + m_ehdr->e_phoff;
    const Elf64_Phdr* phdr = (const Elf64_Phdr*)(phdr_base + index * m_ehdr->e_phentsize);
    return phdr;
}



const Elf64_Shdr* Elf64Loader::GetSectionHeader(int index) const
{
    const char* shdr_base = m_image + m_ehdr->e_shoff;
    const Elf64_Shdr* shdr = (const Elf64_Shdr*)(shdr_base + index * m_ehdr->e_shentsize);
    return shdr;
}



void* Elf64Loader::Load(char* memory)
{
    LoadProgramHeaders(memory);
    ApplyRelocations(memory);

    const uint64_t memoryOffset = (uint64_t)(uintptr_t)memory - m_startAddress;
    void* entry = (void*)(uintptr_t)(m_ehdr->e_entry + memoryOffset);
    return entry;
}



void Elf64Loader::LoadProgramHeaders(char* memory)
{
    for (int i = 0; i != m_ehdr->e_phnum; ++i)
    {
        const Elf64_Phdr* phdr = GetProgramHeader(i);

        if (phdr->p_type != PT_LOAD)
            continue;

        if (phdr->p_filesz != 0)
        {
            const char* src = m_image + phdr->p_offset;
            void* dst = memory + (phdr->p_paddr - m_startAddress);
            memcpy(dst, src, phdr->p_filesz);
        }

        if (phdr->p_memsz > phdr->p_filesz)
        {
            void* dst = memory + (phdr->p_paddr - m_startAddress) + phdr->p_filesz;
            const size_t count = phdr->p_memsz - phdr->p_filesz;
            memset(dst, 0, count);
        }
    }
}



void Elf64Loader::ApplyRelocations(char* memory)
{
    const uint64_t memoryOffset = (uint64_t)(uintptr_t)memory - m_startAddress;

    for (int i = 0; i != m_ehdr->e_shnum; ++i)
    {
        const Elf64_Shdr* shdr = GetSectionHeader(i);

        if (shdr->sh_type != SHT_RELA)
            continue;

        const Elf64_Shdr* symbols_section = GetSectionHeader(shdr->sh_link);
        const char* symbols_base = m_image + symbols_section->sh_offset;

        const char* rel_base = m_image + shdr->sh_offset;

        for (int j = 0; j != (int)(shdr->sh_size / shdr->sh_entsize); ++j)
        {
            const Elf64_Rela* rel = (const Elf64_Rela*)(rel_base + j * shdr->sh_entsize);

            const int sym = ELF64_R_SYM(rel->r_info);
            const int type = ELF64_R_TYPE(rel->r_info);

            const Elf64_Sym* symbol = (const Elf64_Sym*)(symbols_base + sym * symbols_section->sh_entsize);

            switch (type)
            {
            case R_X86_64_GLOB_DAT:
                // Symbol value
                *(uint64_t*)(memory + rel->r_offset - m_startAddress) = symbol->st_value + memoryOffset;
                break;

            default:
                printf("Elf64Loader: unknown relocation type %d!\n", type);
                break;
            }
        }
    }
}
