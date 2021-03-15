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

#include "elfloader.hpp"
#include <cstring>
#include "boot.hpp"
#include "vmm.hpp"


ElfLoader::ElfLoader(const void* elfImage, size_t elfImageSize)
:   m_image((const char*)elfImage),
    m_ehdr(nullptr)
{
    if (elfImageSize < sizeof(Elf_Ehdr))
    {
        return;
    }

    const Elf_Ehdr* ehdr = (const Elf_Ehdr*)elfImage;

    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
        ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
        ehdr->e_ident[EI_MAG3] != ELFMAG3 ||
        ehdr->e_ident[EI_DATA] != ELFDATA2LSB)
    {
        return;
    }

    if (ehdr->e_ident[EI_CLASS] != ELF_CLASS ||
        ehdr->e_machine != ELF_MACHINE ||
        ehdr->e_version != EV_CURRENT)
    {
        return;
    }

    // ELF looks valid...
    m_ehdr = ehdr;
}



const ElfLoader::Elf_Phdr* ElfLoader::GetProgramHeader(int index) const
{
    const char* phdr_base = m_image + m_ehdr->e_phoff;
    const Elf_Phdr* phdr = (const Elf_Phdr*)(phdr_base + index * m_ehdr->e_phentsize);
    return phdr;
}



uint64_t ElfLoader::Load()
{
    if (!LoadProgramHeaders())
    {
        return 0;
    }

    if (m_ehdr->e_type == ET_EXEC)
    {
        return m_ehdr->e_entry;
    }

    Fatal("Unsupported elf type: %d\n", m_ehdr->e_type);
}



bool ElfLoader::LoadProgramHeaders()
{
    for (int i = 0; i != m_ehdr->e_phnum; ++i)
    {
        const Elf_Phdr* phdr = GetProgramHeader(i);

        if (phdr->p_type != PT_LOAD)
            continue;

        // Determine page flags
        physaddr_t flags = PAGE_PRESENT;
        if (phdr->p_flags & PF_W) flags |= PAGE_WRITE;
        if (!(phdr->p_flags & PF_X)) flags |= PAGE_NX;

        // The file size stored in the ELF file is not rounded up to the next page
        const uintptr_t fileSize = align_up(phdr->p_filesz, MEMORY_PAGE_SIZE);

        // Map pages read from the ELF file
        if (fileSize > 0)
        {
            const auto physicalAddress = (uintptr_t)(m_image + phdr->p_offset);
            const auto virtualAddress = phdr->p_vaddr;

            vmm_map(physicalAddress, virtualAddress, fileSize, flags);

            // Not sure if I need to clear the rest of the last page, but I'd rather play safe.
            if (phdr->p_memsz > phdr->p_filesz)
            {
                const auto bytes = fileSize - phdr->p_filesz;
                memset((void*)(physicalAddress + phdr->p_filesz), 0, bytes);
            }
        }

        // The memory size stored in the ELF file is not rounded up to the next page
        const uintptr_t memorySize = align_up(phdr->p_memsz, MEMORY_PAGE_SIZE);

        // Allocate and map zero pages as needed
        if (memorySize > fileSize)
        {
            const auto zeroSize = memorySize - fileSize;
            const auto physicalAddress = g_memoryMap.AllocatePages(MemoryType::Kernel, zeroSize >> MEMORY_PAGE_SHIFT);
            const auto virtualAddress = phdr->p_vaddr + fileSize;

            memset((void*)physicalAddress, 0, zeroSize);

            vmm_map(physicalAddress, virtualAddress, zeroSize, flags);
        }
    }

    return true;
}
