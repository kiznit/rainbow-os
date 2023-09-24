/*
    Copyright (c) 2023, Thierry Tremblay
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

#include "Scheduler.hpp"
#include "Task.hpp"
#include "acpi/Acpi.hpp"
#include "arch.hpp"
#include "display.hpp"
#include "interrupt.hpp"
#include "memory.hpp"
#include "pci.hpp"
#include "uefi.hpp"
#include <metal/log.hpp>
#include <rainbow/boot.hpp>

static Scheduler g_scheduler;
static std::unique_ptr<Acpi> g_acpi;

static void Task2Entry(Task* task, const void* /*args*/)
{
    assert(task->GetId() == 2);
    assert(task->GetState() == TaskState::Running);

    for (;;)
    {
        // MTL_LOG(Info) << "Task 2";
        g_scheduler.Yield();
    }
}

static void Task1Entry(Task* task, const void* /*args*/)
{
    MTL_LOG(Info) << "[KRNL] Hello this is task 1";

    assert(task->GetId() == 1);
    assert(task->GetState() == TaskState::Running);

    // Free boot stack
    extern const char _boot_stack_top[];
    extern const char _boot_stack[];
    VirtualFree((void*)_boot_stack_top, _boot_stack - _boot_stack_top);

    g_scheduler.AddTask(new Task(Task2Entry, nullptr));

    // TODO: this task should return (and die), but we can't until we can idle the processor.
    for (;;)
    {
        // MTL_LOG(Info) << "Task 1";
        g_scheduler.Yield();
    }
}

[[noreturn]] void KernelMain(const BootInfo& bootInfo)
{
    ArchInitialize();

    MTL_LOG(Info) << "[KRNL] Rainbow OS kernel starting";

    // Make sure to call UEFI's SetVirtualMemoryMap() while we have the UEFI boot services still mapped in the lower 4 GB.
    // This is to work around buggy runtime firmware that call into boot services during a call to SetVirtualMemoryMap().
    UefiInitialize(*reinterpret_cast<efi::SystemTable*>(bootInfo.uefiSystemTable));

    // Once UEFI is initialized, it is safe to release boot services code and data.
    MemoryInitialize();

    if (auto rsdp = UefiFindAcpiRsdp())
    {
        g_acpi = std::make_unique<Acpi>();
        if (!g_acpi)
        {
            MTL_LOG(Fatal) << "[KRNL] Unable to allocate memory for ACPI";
            std::abort();
        }

        auto result = g_acpi->Initialize(*rsdp);
        if (!result)
        {
            MTL_LOG(Fatal) << "[KRNL] Failed to initialize ACPI: " << result.error();
            std::abort();
        }
    }

    auto result = InterruptInitialize(g_acpi.get());
    if (!result)
    {
        MTL_LOG(Fatal) << "[KRNL] Could not initialize interrupts: " << result.error();
        std::abort();
    }

    if (g_acpi)
    {
        // TODO: we should use AcpiInterruptModel::PIC if APIC mode is not being used
        // TODO: we might want to do this right after InterruptInitialize(), or even within in.
        auto result = g_acpi->Enable(Acpi::InterruptModel::APIC);
        if (!result)
        {
            MTL_LOG(Fatal) << "[KRNL] Could not initialize ACPI: " << result.error();
            std::abort();
        }
    }

    PciInitialize(g_acpi.get());

    DisplayInitialize();

    // TODO: at this point we can reclaim AcpiReclaimable memory (?)

    g_scheduler.Initialize(new Task(Task1Entry, nullptr));
}
