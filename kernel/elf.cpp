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

#include "elf.hpp"
#include <rainbow/elf.h>
#include <kernel/config.hpp>
#include <kernel/kernel.hpp>
#include <kernel/vdso.hpp>


#if defined(__i386__)
#define MACHINE EM_386
#elif defined(__x86_64__)
#define MACHINE EM_X86_64
#elif defined(__arm__)
#define MACHINE EM_ARM
#elif defined(__aarch64__)
#define MACHINE EM_AARCH64
#endif

#if defined(__i386__) || defined(__arm__)
#define ELFCLASS ELFCLASS32
typedef Elf32_Ehdr Elf_Ehdr;
typedef Elf32_Phdr Elf_Phdr;
#elif defined(__x86_64__) || defined(__aarch64__)
#define ELFCLASS ELFCLASS64
typedef Elf64_Ehdr Elf_Ehdr;
typedef Elf64_Phdr Elf_Phdr;
#endif


static bool IsValid(const Elf_Ehdr* ehdr, physaddr_t elfImageSize)
{
    if (elfImageSize < sizeof(*ehdr))
    {
        Log("ELF image is too small (%X)\n", elfImageSize);
        return false;
    }

    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
        ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
        ehdr->e_ident[EI_MAG3] != ELFMAG3 ||
        ehdr->e_ident[EI_DATA] != ELFDATA2LSB)
    {
        Log("ELF signature not recognied\n");
        return false;
    }

    if (ehdr->e_ident[EI_CLASS] != ELFCLASS ||
        ehdr->e_machine != MACHINE ||
        ehdr->e_version != EV_CURRENT)
    {
        Log("ELF machine/version not supported\n");
        return false;
    }

    if (ehdr->e_type != ET_EXEC)
    {
        Log("ELF image type not supported\n");
        return false;
    }

    //Log("ELF image appears valid\n");
    return true;
}


physaddr_t elf_map(PageTable* pageTable, physaddr_t elfAddress, physaddr_t elfSize)
{
    //Log("elf_map: %X, %X\n", elfAddress, elfSize);

    // Map the ELF header somewhere so that we can read it
    // TODO: mapping this to user space probably doesn't make sense. Can we map it temporarely in kernel space?
    // TODO: we assume the elf header and program headers all fit in one page, this might not be so...
#if defined(__i386__)
    const char* elfImage = (char*)0xD0000000; // TODO: see above comments...
#elif defined(__x86_64__)
    const char* elfImage = (char*)0x0000700000000000; // TODO: see above comments...
#endif
    pageTable->MapPages(elfAddress, elfImage, 1, PAGE_PRESENT | PAGE_NX);

    // Validate the elf image
    const Elf_Ehdr* ehdr = (const Elf_Ehdr*)elfImage;

    if (!IsValid(ehdr, elfSize))
    {
        return 0;
    }

    // Map ELF image in user space
    const char* phdr_base = elfImage + ehdr->e_phoff;
    for (int i = 0; i != ehdr->e_phnum; ++i)
    {
        const Elf_Phdr* phdr = (const Elf_Phdr*)(phdr_base + i * ehdr->e_phentsize);

        if (phdr->p_type != PT_LOAD)
            continue;

        // Determine page flags
        physaddr_t flags = PAGE_PRESENT | PAGE_USER;
        if (phdr->p_flags & PF_W) flags |= PAGE_WRITE;
        if (!(phdr->p_flags & PF_X)) flags |= PAGE_NX;

        // The file size stored in the ELF file is not rounded up to the next page
        const physaddr_t fileSize = align_up(phdr->p_filesz, MEMORY_PAGE_SIZE);

        // Map pages read from the ELF file
        if (fileSize > 0)
        {
            const auto frames = elfAddress + phdr->p_offset;
            const auto address = phdr->p_vaddr;
//TODO: better make sure this isn't mapping things in kernel space!
            pageTable->MapPages(frames, (void*)address, fileSize >> MEMORY_PAGE_SHIFT, flags);
        }

        // The memory size stored in the ELF file is not rounded up to the next page
        const physaddr_t memorySize = align_up(phdr->p_memsz, MEMORY_PAGE_SIZE);

        // Allocate and map zero pages as needed
        if (memorySize > fileSize)
        {
            const auto zeroSize = memorySize - fileSize;
            const auto frames = g_pmm->AllocateFrames(zeroSize >> MEMORY_PAGE_SHIFT);
            const auto address = phdr->p_vaddr + fileSize;
//TODO: better make sure this isn't mapping things in kernel space!
            pageTable->MapPages(frames, (void*)address, zeroSize >> MEMORY_PAGE_SHIFT, flags);
        }

        // Zero out memory as needed
        if (phdr->p_memsz > phdr->p_filesz)
        {
            const auto address = phdr->p_vaddr + phdr->p_filesz;
            memset((void*)address, 0, phdr->p_memsz - phdr->p_filesz);
        }
    }

    // TODO: Temp hack until we have proper VDSO
    // TODO: split .vdso into .vdso.text and .vdso.rodata for better page protection
    const auto vdsoAddress = pageTable->GetPhysicalAddress(&g_vdso);
    pageTable->MapPages(vdsoAddress, VMA_VDSO_START, 1, PAGE_PRESENT | PAGE_USER);

    return ehdr->e_entry;
}
