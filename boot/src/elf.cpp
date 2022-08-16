/*
    Copyright (c) 2022, Thierry Tremblay
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
#include "PageTable.hpp"
#include "boot.hpp"
#include <cstring>
#include <elf.h>
#include <metal/helpers.hpp>
#include <metal/log.hpp>

#if __x86_64__
typedef Elf64_Ehdr Elf_Ehdr;
typedef Elf64_Phdr Elf_Phdr;
inline constexpr uint8_t kElfClass = ELFCLASS64;
inline constexpr uint16_t kElfMachine = EM_X86_64;
#elif __aarch64__
typedef Elf64_Ehdr Elf_Ehdr;
typedef Elf64_Phdr Elf_Phdr;
inline constexpr uint8_t kElfClass = ELFCLASS64;
inline constexpr uint16_t kElfMachine = EM_AARCH64;
#endif

void* ElfLoad(const Module& module, PageTable& pageTable)
{
    // Validate the ELF header
    if (module.size < sizeof(Elf_Ehdr))
    {
        MTL_LOG(Warning) << "Invalid ELF file";
        return nullptr;
    }

    const auto image = reinterpret_cast<const char*>(module.address);
    const auto& ehdr = *reinterpret_cast<const Elf_Ehdr*>(image);
    if (ehdr.e_ident[EI_MAG0] != ELFMAG0 || ehdr.e_ident[EI_MAG1] != ELFMAG1 || ehdr.e_ident[EI_MAG2] != ELFMAG2 ||
        ehdr.e_ident[EI_MAG3] != ELFMAG3 || ehdr.e_ident[EI_DATA] != ELFDATA2LSB)
    {
        MTL_LOG(Warning) << "Invalid ELF file";
        return nullptr;
    }

    // Verify that this ELF file is for the current architecture
    if (ehdr.e_ident[EI_CLASS] != kElfClass || ehdr.e_machine != kElfMachine || ehdr.e_version != EV_CURRENT)
    {
        MTL_LOG(Warning) << "ELF file not for the current architecture";
        return nullptr;
    }

    // We only support executables
    if (ehdr.e_type != ET_EXEC)
    {
        MTL_LOG(Warning) << "ELF file is not an executable";
        return nullptr;
    }

    // Parse program headers
    for (int i = 0; i != ehdr.e_phnum; ++i)
    {
        const auto& phdr = *reinterpret_cast<const Elf_Phdr*>(image + ehdr.e_phoff + i * ehdr.e_phentsize);

        if (phdr.p_type != PT_LOAD)
            continue;

        if (!mtl::IsAligned(phdr.p_offset, mtl::kMemoryPageSize) || !mtl::IsAligned(phdr.p_vaddr, mtl::kMemoryPageSize))
        {
            MTL_LOG(Warning) << "ELF program header " << i << " is not page aligned";
            return nullptr;
        }

        // Determine page attributes
        mtl::PageType pageType;

        if (phdr.p_flags & PF_X)
            pageType = mtl::PageType::KernelCode;
        else if (phdr.p_flags & PF_W)
            pageType = mtl::PageType::KernelData_RW;
        else
            pageType = mtl::PageType::KernelData_RO;

        // The file size stored in the ELF file is not rounded up to the next page
        const auto fileSize = mtl::AlignUp<uintptr_t>(phdr.p_filesz, mtl::kMemoryPageSize);
        if (fileSize > 0)
        {
            const auto physicalAddress = reinterpret_cast<uintptr_t>(image + phdr.p_offset);
            const auto virtualAddress = phdr.p_vaddr;

            pageTable.Map(physicalAddress, virtualAddress, fileSize >> mtl::kMemoryPageShift,
                          static_cast<mtl::PageFlags>(pageType));

            // Not sure if I need to clear the rest of the last page, but I'd rather play safe.
            if (phdr.p_memsz > phdr.p_filesz)
            {
                const auto bytes = fileSize - phdr.p_filesz;
                memset(reinterpret_cast<void*>(physicalAddress + phdr.p_filesz), 0, bytes);
            }
        }

        // The memory size stored in the ELF file is not rounded up to the next page
        const auto memorySize = mtl::AlignUp<uintptr_t>(phdr.p_memsz, mtl::kMemoryPageSize);
        if (memorySize > fileSize)
        {
            const auto zeroSize = memorySize - fileSize;
            const auto physicalAddress = AllocateZeroedPages(zeroSize >> mtl::kMemoryPageShift);
            const auto virtualAddress = phdr.p_vaddr + fileSize;
            pageTable.Map(physicalAddress, virtualAddress, zeroSize >> mtl::kMemoryPageShift,
                          static_cast<mtl::PageFlags>(pageType));
        }
    }

    return (void*)(uintptr_t)ehdr.e_entry;
}