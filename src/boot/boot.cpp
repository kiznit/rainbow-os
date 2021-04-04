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

#include <cstring>
#include <inttypes.h>
#include "vmm.hpp"
#include "boot.hpp"
#include "display.hpp"
#include "elfloader.hpp"
#include "graphics/graphicsconsole.hpp"

#include <kernel/config.hpp>

#if defined(__i386__) || defined(__x86_64__)
#include <metal/x86/cpu.hpp>
#include <metal/x86/cpuid.hpp>
#endif


// Globals
IBootServices* g_bootServices;
IConsole* g_console;
MemoryMap g_memoryMap;
Surface g_framebuffer;
GraphicsConsole g_graphicsConsole;

static BootInfo g_bootInfo;

extern "C" int jumpToKernel(physaddr_t kernelEntryPoint, BootInfo* bootInfo, void* pageTable);


bool CheckArch();



static void InitDisplays(IBootServices* bootServices)
{
    const auto displayCount = bootServices->GetDisplayCount();
    if (displayCount <= 0)
    {
        Fatal("Could not find any usable graphics display\n");
    }

    Log("    Found %d display(s)\n", displayCount);

    for (auto i = 0; i != displayCount; ++i)
    {
        auto display = bootServices->GetDisplay(i);
        SetBestMode(display);

        if (g_bootInfo.framebufferCount < ARRAY_LENGTH(BootInfo::framebuffers))
        {
            auto fb = &g_bootInfo.framebuffers[g_bootInfo.framebufferCount++];
            display->GetFramebuffer(fb);
        }
    }

    // Initialize the graphics console
    if (g_bootInfo.framebufferCount > 0)
    {
        const auto fb = g_bootInfo.framebuffers;

        g_framebuffer.width = fb->width;
        g_framebuffer.height = fb->height;
        g_framebuffer.pitch = fb->pitch;
        g_framebuffer.pixels = (void*)fb->pixels;
        g_framebuffer.format = fb->format;

        g_graphicsConsole.Initialize(&g_framebuffer, &g_framebuffer);
        g_graphicsConsole.Clear();

        g_console = &g_graphicsConsole;
    }
}


static bool LoadModule(IBootServices* bootServices, const char* name, Module& module)
{
    Log("Loading module \"%s\"", name);
    auto length = strlen(name);
    while (length++ < 8) Log(" ");
    Log(": ");

    if (!bootServices->LoadModule(name, module))
    {
        Log("FAILED\n");
        return false;
    }

    Log("address %p, size %08lx\n", (void*)module.address, (size_t)module.size);

    return true;
}


static physaddr_t LoadKernel(const Module& kernel)
{
    ElfLoader elf((void*)kernel.address, kernel.size);

    if (!elf.Valid())
    {
        Fatal("Unsupported: kernel is not a valid elf file\n");
    }

    if (elf.GetType() != ET_EXEC)
    {
        Fatal("Unsupported: kernel is not an executable\n");
    }

#if defined(KERNEL_IA32)
    if (elf.GetMachine() != EM_386)
#elif defined(KERNEL_X86_64)
    if (elf.GetMachine() != EM_X86_64)
#elif defined(KERNEL_ARM)
    if (elf.GetMachine() != EM_ARM)
#elif defined(KERNEL_AARCH64)
    if (elf.GetMachine() != EM_AARCH64)
#endif
    {
        Fatal("Unsupported: kernel architecture (%d)\n", elf.GetMachine());
    }

    vmm_init();

    const physaddr_t entry = elf.Load();
    if (entry == 0)
    {
        Fatal("Error loading kernel\n");
    }

    // The kernel is currently mapped as MemoryType::Bootloader.
    // We want to change this to MemoryType::Kernel.
    g_memoryMap.AddBytes(MemoryType::Kernel, MemoryFlags::None, kernel.address, kernel.size);

    return entry;
}


// We want to make sure the framebuffer is mapped outside the kernel space.
static void RemapConsoleFramebuffer()
{
    if (g_bootInfo.framebufferCount == 0)
    {
        return;
    }

    Framebuffer* fb = &g_bootInfo.framebuffers[0];
    const physaddr_t start = fb->pixels;
    const size_t size = fb->height * fb->pitch;

#if defined(__i386__) || defined(__x86_64__)
    // Setup write combining in PAT entry 4 (PAT4)
    // TODO: this is arch specific!
    const uint64_t pats =
        (x86::PAT_WRITE_BACK       << 0) |
        (x86::PAT_WRITE_THROUGH    << 8) |
        (x86::PAT_UNCACHEABLE_WEAK << 16) |
        (x86::PAT_UNCACHEABLE      << 24) |
        (uint64_t(x86::PAT_WRITE_COMBINING) << 32);

    x86_write_msr(Msr::IA32_PAT, pats);
#endif

    vmm_map(start, (uintptr_t)VMA_FRAMEBUFFER_START, size, PageType::VideoFramebuffer);
}


static void InitAcpi(IBootServices* bootServices)
{
    auto rsdp = bootServices->FindAcpiRsdp();

    g_bootInfo.acpiRsdp = (uintptr_t)rsdp;

    if (!rsdp)
    {
        Log("ACPI RSDP: not found\n\n");
        return;
    }

    Log("ACPI RSDP: %08lx\n", (uintptr_t)rsdp);
    Log("    signature: %c%c%c%c%c%c%c%c\n",
        rsdp->signature[0],
        rsdp->signature[1],
        rsdp->signature[2],
        rsdp->signature[3],
        rsdp->signature[4],
        rsdp->signature[5],
        rsdp->signature[6],
        rsdp->signature[7]
    );
    Log("    oemid    : %c%c%c%c%c%c\n",
        rsdp->oemId[0],
        rsdp->oemId[1],
        rsdp->oemId[2],
        rsdp->oemId[3],
        rsdp->oemId[4],
        rsdp->oemId[5]
    );

    Log("    revision : %d\n", rsdp->revision);
    Log("    rsdt     : %08x\n", rsdp->rsdtAddress);
    if (rsdp->revision >= 2)
    {
        Log("    xsdt     : %016jX\n", ((Acpi::Rsdp20*)rsdp)->xsdtAddress);
    }
    Log("\n");
}


void Boot(IBootServices* bootServices)
{
    g_bootServices = bootServices;

    memset(&g_bootInfo, 0, sizeof(g_bootInfo));
    g_bootInfo.version = RAINBOW_BOOT_VERSION;

    Log("Checking system...\n");
    if (CheckArch())
        Log("Your system meets the requirements to run Rainbow OS\n");
    else
        Fatal("Your system does not meet the requirements to run Rainbow OS\n");


    Log("\nBooting...\n");

    InitDisplays(bootServices);

    g_console->Rainbow();

    Log(" booting...\n\n");

    InitAcpi(bootServices);

    Module kernel;
    LoadModule(bootServices, "kernel", kernel);
    LoadModule(bootServices, "go", g_bootInfo.go);
    LoadModule(bootServices, "logger", g_bootInfo.logger);

    Log("\nExiting boot services\n");
    bootServices->Exit(g_memoryMap);
    bootServices = nullptr;
    g_bootServices = nullptr;

    // Load kernel
    const auto kernelEntryPoint = LoadKernel(kernel);

    // Make sure the framebuffer is accessible to the kernel during initialization
    RemapConsoleFramebuffer();

    // Prepare boot info - do this last!
    g_memoryMap.Sanitize();
    //g_memoryMap.Print(); for (;;);
    g_bootInfo.descriptorCount = g_memoryMap.size();
    g_bootInfo.descriptors = (uintptr_t)g_memoryMap.data();

    // Last bits before jumping to kernel
    Log("\nJumping to kernel at %jX...\n", kernelEntryPoint);

    const int exitCode = jumpToKernel(kernelEntryPoint, &g_bootInfo, vmm_get_pagetable());

    Fatal("Kernel exited with code %d\n", exitCode);

    for(;;);
}
