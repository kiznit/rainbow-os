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

#ifndef _RAINBOW_BOOT_BOOT_HPP
#define _RAINBOW_BOOT_BOOT_HPP

#include <stddef.h>
#include <metal/arch.hpp>
#include <metal/helpers.hpp>
#include <metal/log.hpp>
#include <rainbow/acpi.hpp>
#include <rainbow/boot.hpp>
#include "memory.hpp"

class IConsole;
class IDisplay;


// TODO: I don't feel like this belongs here...
#if defined(KERNEL_IA32)
static const physaddr_t KERNEL_ADDRESS = 0xF0000000;
#elif defined(KERNEL_X86_64)
static const physaddr_t KERNEL_ADDRESS = 0xFFFF800000000000ull;
#endif


class IBootServices
{
public:

    // Allocate memory pages of size MEMORY_PAGE_SIZE.
    // 'maxAddress' is exclusive (all memory will be below that address)
    // On failure, this should call Fatal() and not return.
    virtual void* AllocatePages(int pageCount, physaddr_t maxAddress = KERNEL_ADDRESS) = 0;

    // Exit boot services. The memory map will be returned.
    // Once you call this, calling any of the other methods is undefined behaviour. Don't do it.
    virtual void Exit(MemoryMap& memoryMap) = 0;

    // Find ACPI Root System Descriptor Pointer (RSDP)
    virtual const Acpi::Rsdp* FindAcpiRsdp() const = 0;

    // Read a character from the console (blocking call)
    // Warning: this might not be available!
    virtual int GetChar() = 0;

    // Displays
    virtual int GetDisplayCount() const = 0;
    virtual IDisplay* GetDisplay(int index) const = 0;

    // Load a module (file)
    virtual bool LoadModule(const char* name, Module& module) const = 0;

    // Early console output
    virtual void Print(const char* string, size_t length) = 0;

    // Reboot the system
    virtual void Reboot() __attribute__((noreturn)) = 0;
};


// Globals
extern IBootServices* g_bootServices;
extern IConsole* g_console;
extern MemoryMap g_memoryMap;


// Boot
void Boot(IBootServices* bootServices);


#endif
