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

#ifndef _RAINBOW_BOOT_MULTIBOOT_HPP
#define _RAINBOW_BOOT_MULTIBOOT_HPP

#include "boot.hpp"
#include "vbedisplay.hpp"
#include "graphics/graphicsconsole.hpp"


struct multiboot_info;
struct multiboot2_info;


class Multiboot : public IBootServices
{
public:

    Multiboot(unsigned int magic, const void* mbi);

private:

    void ParseMultibootInfo(const multiboot_info* mbi);
    void ParseMultibootInfo(const multiboot2_info* mbi);
    void InitConsole();

    // IBootServices
    void* AllocatePages(int pageCount, physaddr_t maxAddress = KERNEL_ADDRESS) override;
    void Exit(MemoryMap& memoryMap) override;
    const Acpi::Rsdp* FindAcpiRsdp() const override;
    int GetChar() override;
    int GetDisplayCount() const override;
    IDisplay* GetDisplay(int index) const override;
    bool LoadModule(const char* name, Module& module) const override;
    void Print(const char* string, size_t length) override;
    void Reboot() override;

    // Data
    const multiboot_info*       m_mbi1;
    const multiboot2_info*      m_mbi2;
    mutable const Acpi::Rsdp*   m_acpiRsdp;
    Surface                     m_framebuffer;
    GraphicsConsole             m_console;
    VbeDisplay                  m_display;
};


#endif
