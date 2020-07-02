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

#include <stdio.h>
#include <acpi.h>


// kiznix: https://github.com/kiznit/kiznix/blob/first_attempt/src/kernel/acpi.c
// managarm: https://github.com/managarm/managarm/blob/5a3c1ce2c146cbadc41ed173b0405016353ca0a8/kernel/thor/system/acpi/glue.cpp


ACPI_STATUS AcpiOsInitialize()
{
    return AE_OK;
}



ACPI_STATUS AcpiOsTerminate()
{
    puts(__FUNCTION__);
    return AE_OK;
}



ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer()
{
    puts(__FUNCTION__);
    return 0;
}



ACPI_STATUS AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES* predefinedObject, ACPI_STRING* newValue)
{
    puts(__FUNCTION__);
    return AE_OK;
}



ACPI_STATUS AcpiOsTableOverride(ACPI_TABLE_HEADER* existingTable, ACPI_TABLE_HEADER** newTable)
{
    puts(__FUNCTION__);
    return AE_OK;
}



void* AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS physicalAddress, ACPI_SIZE length)
{
    puts(__FUNCTION__);
    return NULL;
}



void AcpiOsUnmapMemory(void* memory, ACPI_SIZE length)
{
    puts(__FUNCTION__);
}



void* AcpiOsAllocate(ACPI_SIZE size)
{
    return malloc(size);
}



void AcpiOsFree(void* memory)
{
    free(memory);
}


ACPI_THREAD_ID AcpiOsGetThreadId()
{
    puts(__FUNCTION__);
    return 0;
}



ACPI_STATUS AcpiOsExecute(ACPI_EXECUTE_TYPE type, ACPI_OSD_EXEC_CALLBACK function, void* context)
{
    puts(__FUNCTION__);
    return AE_ERROR;
}



void AcpiOsSleep(UINT64 milliseconds)
{
    puts(__FUNCTION__);
}



void AcpiOsStall(UINT32 microseconds)
{
    puts(__FUNCTION__);
}



ACPI_STATUS AcpiOsCreateMutex(ACPI_MUTEX* handle)
{
    puts(__FUNCTION__);
    return AE_ERROR;
}



void AcpiOsDeleteMutex(ACPI_MUTEX handle)
{
    puts(__FUNCTION__);
}



ACPI_STATUS AcpiOsAcquireMutex(ACPI_MUTEX handle, UINT16 timeout)
{
    puts(__FUNCTION__);
    return AE_ERROR;
}



void AcpiOsReleaseMutex(ACPI_MUTEX handle)
{
    puts(__FUNCTION__);
}



ACPI_STATUS AcpiOsCreateSemaphore(UINT32 maxCount, UINT32 initialCount, ACPI_SEMAPHORE* handle)
{
    puts(__FUNCTION__);
    return AE_ERROR;
}



ACPI_STATUS AcpiOsDeleteSemaphore(ACPI_SEMAPHORE handle)
{
    puts(__FUNCTION__);
    return AE_ERROR;
}



ACPI_STATUS AcpiOsWaitSemaphore(ACPI_SEMAPHORE handle, UINT32 count, UINT16 timeout)
{
    puts(__FUNCTION__);
    return AE_ERROR;
}



ACPI_STATUS AcpiOsSignalSemaphore(ACPI_SEMAPHORE handle, UINT32 count)
{
    puts(__FUNCTION__);
    return AE_ERROR;
}



ACPI_STATUS AcpiOsCreateLock(ACPI_SPINLOCK* handle)
{
    puts(__FUNCTION__);
    return AE_ERROR;
}



void AcpiOsDeleteLock(ACPI_SPINLOCK handle)
{
    puts(__FUNCTION__);
}



ACPI_CPU_FLAGS AcpiOsAcquireLock(ACPI_SPINLOCK handle)
{
    puts(__FUNCTION__);
    return 0;
}



void AcpiOsReleaseLock(ACPI_SPINLOCK handle, ACPI_CPU_FLAGS flags)
{
    puts(__FUNCTION__);
}



UINT32 AcpiOsInstallInterruptHandler(UINT32 interruptNumber, ACPI_OSD_HANDLER serviceRoutine, void* context)
{
    puts(__FUNCTION__);
    return AE_ERROR;
}



ACPI_STATUS AcpiOsRemoveInterruptHandler(UINT32 interruptNumber, ACPI_OSD_HANDLER serviceRoutine)
{
    puts(__FUNCTION__);
    return AE_ERROR;
}



ACPI_STATUS AcpiOsReadMemory(ACPI_PHYSICAL_ADDRESS address, UINT64* value, UINT32 width)
{
    puts(__FUNCTION__);
    return AE_ERROR;
}



ACPI_STATUS AcpiOsWriteMemory(ACPI_PHYSICAL_ADDRESS address, UINT64 value, UINT32 width)
{
    puts(__FUNCTION__);
    return AE_ERROR;
}



ACPI_STATUS AcpiOsReadPort(ACPI_IO_ADDRESS address, UINT32* value, UINT32 width)
{
    puts(__FUNCTION__);
    return AE_ERROR;
}



ACPI_STATUS AcpiOsWritePort(ACPI_IO_ADDRESS address, UINT32 value, UINT32 width)
{
    puts(__FUNCTION__);
    return AE_ERROR;
}



ACPI_STATUS AcpiOsReadPciConfiguration(ACPI_PCI_ID* pciId, UINT32 reg, UINT64* result, UINT32 width)
{
    puts(__FUNCTION__);
    return AE_ERROR;
}



ACPI_STATUS AcpiOsWritePciConfiguration(ACPI_PCI_ID* pciId, UINT32 reg, UINT64 value, UINT32 width)
{
    puts(__FUNCTION__);
    return AE_ERROR;
}



void AcpiOsPrintf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}



void AcpiOsVprintf(const char* format, va_list args)
{
    vprintf(format, args);
}



UINT64 AcpiOsGetTimer()
{
    puts(__FUNCTION__);
    return 0;
}



ACPI_STATUS AcpiOsSignal(UINT32 function, void* info)
{
    puts(__FUNCTION__);
    return AE_ERROR;
}



void AcpiOsWaitEventsComplete()
{
    puts(__FUNCTION__);
}



ACPI_STATUS AcpiOsPhysicalTableOverride(ACPI_TABLE_HEADER* existingTable, ACPI_PHYSICAL_ADDRESS* newAddress, UINT32* newTableLength)
{
    puts(__FUNCTION__);
    return AE_ERROR;
}



ACPI_STATUS AcpiOsEnterSleep(UINT8 SleepState, UINT32 RegaValue, UINT32 RegbValue)
{
    puts(__FUNCTION__);
    return AE_ERROR;
}
