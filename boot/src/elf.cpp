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
#include <cstring>
#include <elf.h>
#include <metal/helpers.hpp>
#include <metal/log.hpp>

#if __x86_64__
typedef Elf64_Ehdr Elf_Ehdr;
typedef Elf64_Phdr Elf_Phdr;
inline constexpr uint8_t ELF_CLASS = ELFCLASS64;
inline constexpr uint16_t ELF_MACHINE = EM_X86_64;
#elif __aarch64__
typedef Elf64_Ehdr Elf_Ehdr;
typedef Elf64_Phdr Elf_Phdr;
inline constexpr uint8_t ELF_CLASS = ELFCLASS64;
inline constexpr uint16_t ELF_MACHINE = EM_AARCH64;
#endif

void* elf_load(const Module& module)
{
    // Validate the ELF header
    if (module.size < sizeof(Elf_Ehdr))
        return nullptr;

    const auto image = reinterpret_cast<const char*>(module.address);
    const auto& ehdr = *reinterpret_cast<const Elf_Ehdr*>(image);
    if (ehdr.e_ident[EI_MAG0] != ELFMAG0 || ehdr.e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr.e_ident[EI_MAG2] != ELFMAG2 || ehdr.e_ident[EI_MAG3] != ELFMAG3 ||
        ehdr.e_ident[EI_DATA] != ELFDATA2LSB)
    {
        return nullptr;
    }

    // Verify that this ELF file is for the current architecture
    if (ehdr.e_ident[EI_CLASS] != ELF_CLASS || ehdr.e_machine != ELF_MACHINE ||
        ehdr.e_version != EV_CURRENT)
    {
        return nullptr;
    }

    // We only support executables
    if (ehdr.e_type != ET_EXEC)
    {
        return nullptr;
    }

    // Parse program headers
    for (int i = 0; i != ehdr.e_phnum; ++i)
    {
        const auto& phdr =
            *reinterpret_cast<const Elf_Phdr*>(image + ehdr.e_phoff + i * ehdr.e_phentsize);

        if (phdr.p_type != PT_LOAD)
            continue;

        // TODO: determine page attributes

        // The file size stored in the ELF file is not rounded up to the next page
        const auto fileSize = mtl::align_up<uintptr_t>(phdr.p_filesz, mtl::MEMORY_PAGE_SIZE);
        if (fileSize > 0)
        {
            const auto physicalAddress = reinterpret_cast<uintptr_t>(image + phdr.p_offset);
            const auto virtualAddress = phdr.p_vaddr;

            // TODO
            MTL_LOG(Info) << "Map " << mtl::hex(physicalAddress) << " to "
                          << mtl::hex(virtualAddress) << " size " << mtl::hex(fileSize);

            // Not sure if I need to clear the rest of the last page, but I'd rather play safe.
            if (phdr.p_memsz > phdr.p_filesz)
            {
                const auto bytes = fileSize - phdr.p_filesz;
                memset(reinterpret_cast<void*>(physicalAddress + phdr.p_filesz), 0, bytes);
            }
        }

        // The memory size stored in the ELF file is not rounded up to the next page
        const auto memorySize = mtl::align_up<uintptr_t>(phdr.p_memsz, mtl::MEMORY_PAGE_SIZE);
        if (memorySize > fileSize)
        {
            const auto zeroSize = memorySize - fileSize;
            const auto physicalAddress = 0ull; // TODO g_memoryMap.AllocatePages(MemoryType::Kernel,
                                               // zeroSize >> MEMORY_PAGE_SHIFT);
            const auto virtualAddress = phdr.p_vaddr + fileSize;

            memset(reinterpret_cast<void*>(physicalAddress), 0, zeroSize);

            // TODO
            MTL_LOG(Info) << "Map " << mtl::hex(physicalAddress) << " to "
                          << mtl::hex(virtualAddress) << " size " << mtl::hex(zeroSize);
        }
    }

    return reinterpret_cast<void*>((uintptr_t)ehdr.e_entry);
}