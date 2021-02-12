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

#ifndef _RAINBOW_BOOT_EFIBOOT_HPP
#define _RAINBOW_BOOT_EFIBOOT_HPP

#include <vector>
#include "boot.hpp"
#include "efifilesystem.hpp"


class EfiDisplay;


class EfiBoot : public IBootServices
{
public:

    EfiBoot(EFI_HANDLE hImage, EFI_SYSTEM_TABLE* systemTable);

private:

    void InitConsole();
    void InitDisplays();

    // IBootServices
    physaddr_t AllocatePages(int pageCount, physaddr_t maxAddress = KERNEL_ADDRESS) override;
    void Exit(MemoryMap& memoryMap) override;
    const Acpi::Rsdp* FindAcpiRsdp() const override;
    int GetChar() override;
    int GetDisplayCount() const override;
    IDisplay* GetDisplay(int index) const override;
    bool LoadModule(const char* name, Module& module) const override;
    void Print(const char* string, size_t length) override;
    void Reboot() override;

    // Data
    EFI_HANDLE              m_hImage;
    EFI_SYSTEM_TABLE*       m_systemTable;
    EFI_BOOT_SERVICES*      m_bootServices;
    EFI_RUNTIME_SERVICES*   m_runtimeServices;
    EfiFileSystem           m_fileSystem;
    std::vector<EfiDisplay> m_displays;
};


#endif
