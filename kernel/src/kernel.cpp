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

#include "Scheduler.hpp"
#include "Task.hpp"
#include "acpi/acpi.hpp"
#include "arch.hpp"
#include "display.hpp"
#include "memory.hpp"
#include "pci.hpp"
#include "uefi.hpp"
#include <metal/log.hpp>
#include <rainbow/boot.hpp>

static Scheduler g_scheduler;

static void Task2Entry(Task* task, const void* /*args*/)
{
    assert(task->GetId() == 2);
    assert(task->GetState() == TaskState::Running);

    for (;;)
    {
        MTL_LOG(Info) << "Task 2";
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

    g_scheduler.AddTask(Task::Create(Task2Entry, nullptr));

    // TODO: this task should return (and die), but we can't until we can idle the processor.
    for (;;)
    {
        MTL_LOG(Info) << "Task 1";
        g_scheduler.Yield();
    }
}

void KernelMain(const BootInfo& bootInfo)
{
    ArchInitialize();

    MTL_LOG(Info) << "[KRNL] Rainbow OS kernel starting";

    // Make sure to call UEFI's SetVirtualMemoryMap() while we have the UEFI boot services still mapped in the lower 4 GB.
    // This is to work around buggy runtime firmware that call into boot services during a call to SetVirtualMemoryMap().
    UefiInitialize(*reinterpret_cast<efi::SystemTable*>(bootInfo.uefiSystemTable));

    // Once UEFI is initialized, it is safe to release boot services code and data.
    MemoryInitialize();

    if (auto rsdp = UefiFindAcpiRsdp())
        AcpiInitialize(*rsdp);

    PciInitialize();

    DisplayInitialize();

    if (AcpiIsInitialized())
    {
        AcpiEnable(AcpiInterruptModel::APIC);
        // AcpiEnumerateNamespace();
    }

    // TODO: at this point we can reclaim AcpiReclaimable memory (?)

    g_scheduler.Initialize(Task::Create(Task1Entry, nullptr));
}
