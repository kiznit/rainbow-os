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

#include <cstring>
#include <metal/helpers.hpp>
#include <metal/log.hpp>

#include "config.hpp"
#include "kernel.hpp"
#include "pmm.hpp"
#include "vdso.hpp"
#include "vmm.hpp"


#if defined(__i386__)
#define MACHINE EM_386
#elif defined(__x86_64__)
#define MACHINE EM_X86_64
#elif defined(__arm__)
#define MACHINE EM_ARM
#elif defined(__aarch64__)
#define MACHINE EM_AARCH64
#endif

#if UINTPTR_MAX == 0xFFFFFFFF
#define ELFCLASS ELFCLASS32
typedef Elf32_Ehdr Elf_Ehdr;
typedef Elf32_Phdr Elf_Phdr;
#elif UINTPTR_MAX == 0xFFFFFFFFFFFFFFFFu
#define ELFCLASS ELFCLASS64
typedef Elf64_Ehdr Elf_Ehdr;
typedef Elf64_Phdr Elf_Phdr;
#endif


static bool IsValid(const Elf_Ehdr* ehdr, physaddr_t elfImageSize)
{
    if (elfImageSize < sizeof(*ehdr))
    {
        Log("ELF image is too small (%jx)\n", elfImageSize);
        return false;
    }

    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
        ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
        ehdr->e_ident[EI_MAG3] != ELFMAG3 ||
        ehdr->e_ident[EI_DATA] != ELFDATA2LSB)
    {
        Log("ELF signature not recognized\n");
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


int elf_map(physaddr_t elfAddress, physaddr_t elfSize, ElfImageInfo& info)
{
    //Log("elf_map: %X, %X\n", elfAddress, elfSize);

    // Map the ELF header somewhere so that we can read it
    // TODO: mapping this to user space probably doesn't make sense. Can we map it temporarely in kernel space?
    // TODO: we assume the elf header and program headers all fit in one page, this might not be so...
#if UINTPTR_MAX == 0xFFFFFFFF
    char* elfImage = (char*)0xD0000000; // TODO: see above comments...
#elif UINTPTR_MAX == 0xFFFFFFFFFFFFFFFFu
    char* elfImage = (char*)0x0000700000000000; // TODO: see above comments...
#endif
    vmm_map_pages(elfAddress, elfImage, 1, PageType::KernelData_RO);

    // Validate the elf image
    const Elf_Ehdr* ehdr = (const Elf_Ehdr*)elfImage;

    if (!IsValid(ehdr, elfSize))
    {
        // TODO: error code?
        return -1;
    }

    info.vmaStart = (void*)-1;
    info.vmaEnd = nullptr;

    // Map ELF image in user space
    const char* phdr_base = elfImage + ehdr->e_phoff;
    for (int i = 0; i != ehdr->e_phnum; ++i)
    {
        const Elf_Phdr* phdr = (const Elf_Phdr*)(phdr_base + i * ehdr->e_phentsize);

        if (phdr->p_type == PT_LOAD)
        {
            // Determine page type
            PageType pageType;

            if (phdr->p_flags & PF_X)
                pageType = PageType::UserCode;
            else if (phdr->p_flags & PF_W)
                pageType = PageType::UserData_RW;
            else
                pageType = PageType::UserData_RO;

            // Update vma range used by program
            info.vmaStart = std::min(info.vmaStart, (void*)phdr->p_vaddr);
            info.vmaEnd   = std::max(info.vmaEnd,   (void*)(phdr->p_vaddr + phdr->p_memsz));

            // PHDR might not start at a page boundary!
            const auto pageOffset = phdr->p_offset & (MEMORY_PAGE_SIZE - 1);
            const auto pageOffset2 = phdr->p_vaddr & (MEMORY_PAGE_SIZE - 1);

            // Make sure both offsets are the same, otherwise we don't know how to load this.
            if (pageOffset != pageOffset2)
            {
                assert(pageOffset == pageOffset2);
                return -1;
            }

            // The file size stored in the ELF file is not rounded up to the next page.
            // It also doesn't take the starting page offset into account.
            const physaddr_t filePages = phdr->p_filesz ? align_up(phdr->p_filesz + pageOffset, MEMORY_PAGE_SIZE) >> MEMORY_PAGE_SHIFT : 0;

            // Map pages read from the ELF file
            if (filePages > 0)
            {
                const auto frames = elfAddress + phdr->p_offset - pageOffset;
                const auto address = phdr->p_vaddr - pageOffset;
//TODO: better make sure this isn't mapping things in kernel space!
                vmm_map_pages(frames, (void*)address, filePages, pageType);
            }

            // The memory size stored in the ELF file is not rounded up to the next page
            const physaddr_t memoryPages = align_up(phdr->p_memsz + pageOffset, MEMORY_PAGE_SIZE) >> MEMORY_PAGE_SHIFT;

            // Allocate and map zero pages as needed
            if (memoryPages > filePages)
            {
                const auto zeroPages = memoryPages - filePages;
                const auto frames = pmm_allocate_frames(zeroPages);
                const auto address = phdr->p_vaddr + filePages * MEMORY_PAGE_SIZE;
//TODO: better make sure this isn't mapping things in kernel space!
                vmm_map_pages(frames, (void*)address, zeroPages, pageType);
            }

            // Zero out memory as needed
            if (phdr->p_memsz > phdr->p_filesz)
            {
                const auto address = phdr->p_vaddr + phdr->p_filesz;
                const auto zeroSize = phdr->p_memsz - phdr->p_filesz;
                memset((void*)address, 0, zeroSize);
            }
        }
        else if (phdr->p_type == PT_PHDR)
        {
            info.phdr  = (void*)(uintptr_t)phdr->p_vaddr;
            info.phent = (void*)(uintptr_t)ehdr->e_phentsize;
            info.phnum = ehdr->e_phnum;
            info.entry = (void*)(uintptr_t)ehdr->e_entry;
        }
    }

    // TODO: Temp hack until we have proper VDSO
    // TODO: split .vdso into .vdso.text and .vdso.rodata for better page protection
    const auto vdsoAddress = vmm_get_physical_address(&g_vdso);
    vmm_map_pages(vdsoAddress, VMA_VDSO_START, 1, PageType::UserCode);

    // Unmap elf header
    vmm_unmap_pages(elfImage, 1);

    // ehdr and other ELF header fields are now invalid (memory was unmapped)

    return 0;
}
