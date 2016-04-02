
/*
    Copyright (c) 2015, Thierry Tremblay
    All rights reserved.

    Redistribution and use source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retathe above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer the documentation
      and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING ANY WAY OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef INCLUDED_EFI_EFI_HPP
#define INCLUDED_EFI_EFI_HPP

#include <stddef.h>
#include <stdint.h>


namespace efi {

/*
    Basic types
*/

typedef uint8_t boolean_t;
typedef intptr_t intn_t;
typedef uintptr_t uintn_t;
typedef uintn_t status_t;
typedef void* handle_t;
typedef void* event_t;
typedef uint64_t lba_t;
typedef uintn_t tpl_t;


struct GUID
{
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t  data4[8];
};


#define EFIAPI __attribute__((ms_abi))


static const boolean_t TRUE = 1;
static const boolean_t FALSE = 0;

static const uint64_t PAGE_SIZE = 4096;
static const uint64_t PAGE_MASK = 0xFFF;
static const int      PAGE_SHIFT = 12;

/*
    Status Codes
*/

#define EFI_ERROR_BIT (((efi::status_t)1) << (sizeof(efi::status_t) * 8 - 1))

#define EFI_ERROR(status) (status & EFI_ERROR_BIT)

#define EFI_SUCCESS                     0

#define EFI_LOAD_ERROR                  (EFI_ERROR_BIT | 1)
#define EFI_INVALID_PARAMETER           (EFI_ERROR_BIT | 2)
#define EFI_UNSUPPORTED                 (EFI_ERROR_BIT | 3)
#define EFI_BAD_BUFFER_SIZE             (EFI_ERROR_BIT | 4)
#define EFI_BUFFER_TOO_SMALL            (EFI_ERROR_BIT | 5)
#define EFI_NOT_READY                   (EFI_ERROR_BIT | 6)
#define EFI_DEVICE_ERROR                (EFI_ERROR_BIT | 7)
#define EFI_WRITE_PROTECTED             (EFI_ERROR_BIT | 8)
#define EFI_OUT_OF_RESOURCES            (EFI_ERROR_BIT | 9)
#define EFI_VOLUME_CORRUPTED            (EFI_ERROR_BIT | 10)
#define EFI_VOLUME_FULL                 (EFI_ERROR_BIT | 11)
#define EFI_NO_MEDIA                    (EFI_ERROR_BIT | 12)
#define EFI_MEDIA_CHANGED               (EFI_ERROR_BIT | 13)
#define EFI_NOT_FOUND                   (EFI_ERROR_BIT | 14)
#define EFI_ACCESS_DENIED               (EFI_ERROR_BIT | 15)
#define EFI_NO_RESPONSE                 (EFI_ERROR_BIT | 16)
#define EFI_NO_MAPPING                  (EFI_ERROR_BIT | 17)
#define EFI_TIMEOUT                     (EFI_ERROR_BIT | 18)
#define EFI_NOT_STARTED                 (EFI_ERROR_BIT | 19)
#define EFI_ALREADY_STARTED             (EFI_ERROR_BIT | 20)
#define EFI_ABORTED                     (EFI_ERROR_BIT | 21)
#define EFI_ICMP_ERROR                  (EFI_ERROR_BIT | 22)
#define EFI_TFTP_ERROR                  (EFI_ERROR_BIT | 23)
#define EFI_PROTOCOL_ERROR              (EFI_ERROR_BIT | 24)
#define EFI_INCOMPATIBLE_VERSION        (EFI_ERROR_BIT | 25)
#define EFI_SECURITY_VIOLATION          (EFI_ERROR_BIT | 26)
#define EFI_CRC_ERROR                   (EFI_ERROR_BIT | 27)
#define EFI_END_OF_MEDIA                (EFI_ERROR_BIT | 28)
#define EFI_END_OF_FILE                 (EFI_ERROR_BIT | 31)
#define EFI_INVALID_LANGUAGE            (EFI_ERROR_BIT | 32)
#define EFI_COMPROMISED_DATA            (EFI_ERROR_BIT | 33)
#define EFI_IP_ADDRESS_CONFLICT


#define EFI_WARN_UNKOWN_GLYPH           1
#define EFI_WARN_DELETE_FAILURE         2
#define EFI_WARN_WRITE_FAILURE          3
#define EFI_WARN_BUFFER_TOO_SMALL       4
#define EFI_WARN_STALE_DATA             5





/*
    Device Path Protocol
*/

struct DevicePathProtocol
{
    static const GUID guid;

    uint8_t type;
    uint8_t subType;
    uint8_t length[2];
};



/*
    Simple Text Input Protocol
*/

struct InputKey
{
    uint16_t scanCode;
    wchar_t unicodeChar;
};


struct SimpleTextInputProtocol
{
    static const GUID guid;

    status_t (EFIAPI* Reset)(SimpleTextInputProtocol* self, boolean_t extendedVerification);
    status_t (EFIAPI* ReadKeyStroke)(SimpleTextInputProtocol* self, InputKey* key);
    event_t waitForKey;
};



/*
    Simple Text Output Protocol
*/

struct SimpleTextOutputMode
{

    int32_t maxMode;
    int32_t mode;
    int32_t attribute;
    int32_t cursorColumn;
    int32_t cursorRow;
    boolean_t cursorVisible;
};


#define EFI_BLACK           0x00
#define EFI_BLUE            0x01
#define EFI_GREEN           0x02
#define EFI_CYAN            0x03
#define EFI_RED             0x04
#define EFI_MAGENTA         0x05
#define EFI_BROWN           0x06
#define EFI_LIGHTGRAY       0x07
#define EFI_DARKGRAY        0x08
#define EFI_LIGHTBLUE       0x09
#define EFI_LIGHTGREEN      0x0A
#define EFI_LIGHTCYAN       0x0B
#define EFI_LIGHTRED        0x0C
#define EFI_LIGHTMAGENTA    0x0D
#define EFI_YELLOW          0x0E
#define EFI_WHITE           0x0F

#define EFI_TEXT_ATTR(Foreground,Background) ((Foreground) | ((Background) << 4))


struct SimpleTextOutputProtocol
{
    static const GUID guid;

    status_t (EFIAPI* Reset)(SimpleTextOutputProtocol* self, boolean_t extendedVerification);
    status_t (EFIAPI* OutputString)(SimpleTextOutputProtocol* self, wchar_t* string);
    status_t (EFIAPI* TestString)(SimpleTextOutputProtocol* self, wchar_t* string);
    status_t (EFIAPI* QueryMode)(SimpleTextOutputProtocol* self, uintn_t modeNumber, uintn_t* columns, uintn_t* rows);
    status_t (EFIAPI* SetMode)(SimpleTextOutputProtocol* self, uintn_t modeNumber);
    status_t (EFIAPI* SetAttribute)(SimpleTextOutputProtocol* self, uintn_t attribute);
    status_t (EFIAPI* ClearScreen)(SimpleTextOutputProtocol* self);
    status_t (EFIAPI* SetCursorPosition)(SimpleTextOutputProtocol* self, uintn_t column, uintn_t row);
    status_t (EFIAPI* EnableCursor)(SimpleTextOutputProtocol* self, boolean_t visible);

    SimpleTextOutputMode* mode;
};



/*
    Table Header
*/
static const uint32_t EFI_REVISION_2_60 = (2<<16) | (60);
static const uint32_t EFI_REVISION_2_50 = (2<<16) | (50);
static const uint32_t EFI_REVISION_2_40 = (2<<16) | (40);
static const uint32_t EFI_REVISION_2_31 = (2<<16) | (31);
static const uint32_t EFI_REVISION_2_30 = (2<<16) | (30);
static const uint32_t EFI_REVISION_2_20 = (2<<16) | (20);
static const uint32_t EFI_REVISION_2_10 = (2<<16) | (10);
static const uint32_t EFI_REVISION_2_00 = (2<<16) | (00);
static const uint32_t EFI_REVISION_1_10 = (1<<16) | (10);
static const uint32_t EFI_REVISION_1_02 = (1<<16) | (02);


struct TableHeader
{
    uint64_t signature;
    uint32_t revision;
    uint32_t headerSize;
    uint32_t crc32;
    uint32_t reserved;
};



/*
    Boot Services
*/

enum AllocateType
{
    AllocateAnyPages,
    AllocateMaxAddress,
    AllocateAddress,
    MaxAllocateType
};


enum MemoryType
{
    EfiReservedMemoryType,
    EfiLoaderCode,
    EfiLoaderData,
    EfiBootServicesCode,
    EfiBootServicesData,
    EfiRuntimeServicesCode,
    EfiRuntimeServicesData,
    EfiConventionalMemory,
    EfiUnusableMemory,
    EfiACPIReclaimMemory,
    EfiACPIMemoryNVS,
    EfiMemoryMappedIO,
    EfiMemoryMappedIOPortSpace,
    EfiPalCode,
    EfiPersistentMemory,
    EfiMaxMemoryType
};


typedef uint64_t PhysicalAddress;
typedef uint64_t VirtualAddress;


struct MemoryDescriptor
{
    uint32_t        type;
    PhysicalAddress physicalStart;
    VirtualAddress  virtualStart;
    uint64_t        numberOfPages;
    uint64_t        attribute;
};


static const uint64_t EFI_MEMORY_UC             = 0x0000000000000001ull;
static const uint64_t EFI_MEMORY_WC             = 0x0000000000000002ull;
static const uint64_t EFI_MEMORY_WT             = 0x0000000000000004ull;
static const uint64_t EFI_MEMORY_WB             = 0x0000000000000008ull;
static const uint64_t EFI_MEMORY_UCE            = 0x0000000000000010ull;
static const uint64_t EFI_MEMORY_WP             = 0x0000000000001000ull;
static const uint64_t EFI_MEMORY_RP             = 0x0000000000002000ull;
static const uint64_t EFI_MEMORY_XP             = 0x0000000000004000ull;
static const uint64_t EFI_MEMORY_NV             = 0x0000000000008000ull;
static const uint64_t EFI_MEMORY_MORE_RELIABLE  = 0x0000000000010000ull;
static const uint64_t EFI_MEMORY_RO             = 0x0000000000020000ull;
static const uint64_t EFI_MEMORY_RUNTIME        = 0x8000000000000000ull;


enum InterfaceType
{
    NativeInterface
};


enum LocateSearchType
{
    AllHandles,
    ByRegisterNotify,
    ByProtocol
};


enum TimerDelay
{
    TimerCancel,
    TimerPeriodic,
    TimerRelative
};


struct OpenProtocolInformationEntry
{
    handle_t agentHandle;
    handle_t controllerHandle;
    uint32_t attributes;
    uint32_t openCount;
};


static const uint32_t OpenProtocol_ByHandleProtocol     = 0x00000001;
static const uint32_t OpenProtocol_GetProtocol          = 0x00000002;
static const uint32_t OpenProtocol_TestProtocol         = 0x00000004;
static const uint32_t OpenProtocol_ByChildController    = 0x00000008;
static const uint32_t OpenProtocol_ByDriver             = 0x00000010;
static const uint32_t OpenProtocol_Exclusive            = 0x00000020;


typedef void (EFIAPI *EventNotify)(event_t event, void* context);


struct BootServices
{
    static const uint64_t SIGNATURE = 0x56524553544f4f42ull;

    TableHeader header;

    // EFI 1.0
    tpl_t (EFIAPI *pRaideTpl)(tpl_t newTpl);
    void (EFIAPI *pRestoreTpl)(tpl_t oldTpl);
    status_t (EFIAPI *pAllocatePages)(AllocateType type, MemoryType memoryType, uintn_t pages, PhysicalAddress* memory);
    status_t (EFIAPI *pFreePages)(PhysicalAddress memory, uintn_t pages);
    status_t (EFIAPI *pGetMemoryMap)(uintn_t* memoryMapSize, MemoryDescriptor* memoryMap, uintn_t* mapKey, uintn_t* descriptorSize, uint32_t* descriptorVersion);
    status_t (EFIAPI *pAllocatePool)(MemoryType poolType, uintn_t size, void** buffer);
    status_t (EFIAPI *pFreePool)(void* buffer);

    status_t (EFIAPI *pCreateEvent)(uint32_t type, tpl_t notifyTpl, EventNotify notifyFunction, void* notifyContext, event_t* event);
    status_t (EFIAPI *pSetTimer)(event_t event, TimerDelay type, uint64_t triggerTime);
    status_t (EFIAPI *pWaitForEvent)(uintn_t numberOfEvents, event_t* event, uintn_t* index);
    status_t (EFIAPI *pSignalEvent)(event_t event);
    status_t (EFIAPI *pCloseEvent)(event_t event);
    status_t (EFIAPI *pCheckEvent)(event_t event);

    status_t (EFIAPI *pInstallProtocolInterface)(handle_t* handle, GUID* protocol, InterfaceType interfaceType, void* interface);
    status_t (EFIAPI *pReinstallProtocolInterface)(handle_t handle, GUID* protocol, void* oldInterface, void* newInterface);
    status_t (EFIAPI *pUninstallProtocolInterface)(handle_t handle, GUID* protocol, void* interface);
    status_t (EFIAPI *pHandleProtocol)(handle_t Handle, const GUID* protocol, void** interface);
    void* reserved;
    status_t (EFIAPI *pRegisterProtocolNotify)(GUID* protocol, event_t event, void** registration);
    status_t (EFIAPI *pLocateHandle)(LocateSearchType searchType, const GUID* protocol, void* searchKey, uintn_t* bufferSize, handle_t* buffer);
    status_t (EFIAPI *pLocateDevicePath)(const GUID* protocol, DevicePathProtocol** devicePath, handle_t* device);

    status_t (EFIAPI *pInstallConfigurationTable)(GUID* guid, void* table);

    status_t (EFIAPI *pLoadImage)(boolean_t bootPolicy, handle_t parentImageHandle, DevicePathProtocol* devicePath, void* sourceBuffer, uintn_t sourceSize, handle_t* imageHandle);
    status_t (EFIAPI *pStartImage)(handle_t imageHandle, uintn_t* exitDataSize, wchar_t** exitData);
    status_t (EFIAPI *pExit)(handle_t imageHandle, status_t exitStatus, uintn_t exitDataSize, wchar_t* exitData);
    status_t (EFIAPI *pUnloadImage)(handle_t imageHandle);
    status_t (EFIAPI *pExitBootServices)(handle_t imageHandle, uintn_t mapKey);

    status_t (EFIAPI *pGetNextMonotonicCount)(uint64_t* count);
    status_t (EFIAPI *pStall)(uintn_t microseconds);
    status_t (EFIAPI *pSetWatchdogTimer)(uintn_t timeout, uint64_t watchdogCode, uintn_t dataSize, wchar_t* watchdogData);

    // EFI 1.1
    status_t (EFIAPI *pConnectController)(handle_t controllerHandle, handle_t* driverImageHandle, DevicePathProtocol* remainingDevicePath, boolean_t recursive);
    status_t (EFIAPI *pDisconnectController)(handle_t controllerHandle, handle_t driverImageHandle, handle_t childHandle);
    status_t (EFIAPI *pOpenProtocol)(handle_t handle, const GUID* protocol, void** interface, handle_t agentHandle, handle_t controllerHandle, uint32_t attributes);
    status_t (EFIAPI *pCloseProtocol)(handle_t handle, const GUID* protocol, handle_t agentHandle, handle_t controllerHandle);
    status_t (EFIAPI *pOpenProtocolInformation)(handle_t handle, const GUID* protocol, OpenProtocolInformationEntry** entryBuffer, uintn_t* entryCount);
    status_t (EFIAPI *pProtocolsPerHandle)(handle_t handle, GUID*** protocolBuffer, uintn_t* protocolBufferCount);
    status_t (EFIAPI *pLocateHandleBuffer)(LocateSearchType searchType, const GUID* protocol, void* searchKey, uintn_t* noHandles, handle_t** buffer);
    status_t (EFIAPI *pLocateProtocol)(GUID* protocol, void* registration, void** interface);
    status_t (EFIAPI *pInstallMultipleProtocolInterfaces)(handle_t* handle, ...);
    status_t (EFIAPI *pIninstallMultipleProtocolInterfaces)(handle_t handle, ...);
    status_t (EFIAPI *pCalculateCrc32)(void* data, uintn_t dataSize, uint32_t* crc32);
    void (EFIAPI *pCopyMem)(void* destination, void* source, uintn_t length);
    void (EFIAPI *pSetMem)(void* buffer, uintn_t size, uint8_t value);

    // UEFI 2.0
    status_t (EFIAPI *pCreateEventEx)(uint32_t type, tpl_t notifyTpl, EventNotify notifyFunction, const void* notifyContext, const GUID* eventGroup, event_t* event);



    // Helpers
    void* Allocate(size_t size)
    {
        void* memory;
        if (EFI_ERROR(pAllocatePool(EfiLoaderData, size, &memory)))
            return NULL;
        return memory;
    }


    void Free(void* memory)
    {
        pFreePool(memory);
    }


    void* AllocatePages(int pageCount, PhysicalAddress maxAddress)
    {
        efi::PhysicalAddress address = maxAddress - 1;
        if (EFI_ERROR(pAllocatePages(AllocateMaxAddress, EfiBootServicesData, pageCount, &address)))
            return NULL;
        return (void*)address;
    }


    MemoryDescriptor* GetMemoryMap(uintn_t* descriptorCount, uintn_t* descriptorSize, uint32_t* descriptorVersion, uintn_t* mapKey);


    status_t WaitForEvent(size_t eventCount, event_t* events, size_t* index)
    {
        return pWaitForEvent(eventCount, events, index);
    }


    status_t LocateHandle(const GUID* protocol, size_t* handleCount, handle_t** handles);

    template<typename Protocol>
    status_t LocateHandle(size_t* handleCount, handle_t** handles)
    {
        return LocateHandle(&Protocol::guid, handleCount, handles);
    }


    template<typename Protocol>
    status_t OpenProtocol(handle_t handle, Protocol** interface)
    {
        if (header.revision < EFI_REVISION_1_10)
            return pHandleProtocol(handle, &Protocol::guid, (void**)interface);
        else
            return pOpenProtocol(handle, &Protocol::guid, (void**)interface, handle, NULL, OpenProtocol_ByHandleProtocol);
    }


    template<typename Protocol>
    status_t CloseProtocol(handle_t handle, Protocol** interface)
    {
        status_t status;

        if (header.revision < EFI_REVISION_1_10)
            status = EFI_SUCCESS;
        else
            status = pCloseProtocol(handle, &Protocol::guid, handle, NULL);

        if (!EFI_ERROR(status))
            *interface = NULL;

        return status;
    }
};



/*
    Runtime Services
*/


static const uint8_t EFI_TIME_ADJUST_DAYLIGHT = 0x01;
static const uint8_t EFI_TIME_IN_DAYLIGHT = 0x02;

static const uint16_t EFI_UNSPECIFIED_TIMEZONE = 0x07FF;

struct Time
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t pad1;
    uint32_t nanosecond;
    int16_t timeZone;
    uint8_t daylight;
    uint8_t pad2;
};


struct TimeCapabilities
{
    uint32_t resolution;
    uint32_t accuracy;
    boolean_t setsToZero;
};


enum ResetType
{
    EfiResetCold,
    EfiResetWarm,
    EfiResetShutdown,
    EfiResetPlatformSpecific
};


struct CapsuleHeader
{
    GUID capsuleGuid;
    uint32_t headerSize;
    uint32_t flags;
    uint32_t capsuleImageSize;
};


struct RuntimeServices
{
    static const uint64_t SIGNATURE = 0x56524553544e5552ull;

    TableHeader header;

    // EFI 1.0

    status_t (EFIAPI *pGetTime)(Time* time, TimeCapabilities* capabilities);
    status_t (EFIAPI* pSetTime)(Time* time);
    status_t (EFIAPI* pGetWakeupTime)(boolean_t* enabled, boolean_t* pending, Time* time);
    status_t (EFIAPI* pSetWakeupTime)(boolean_t enable, Time* time);
    status_t (EFIAPI* pSetVirtualAddressMap)(uintn_t memoryMapSize, uintn_t descriptorSize, uint32_t descriptorVersion, MemoryDescriptor* virtualMap);
    status_t (EFIAPI* pConvertPointer)(uintn_t debugDisposition, void** address);
    status_t (EFIAPI* pGetVariable)(wchar_t* variableName, GUID* vendorGuid, uint32_t* attributes, uintn_t* dataSize, void* data);
    status_t (EFIAPI* pGetNextVariableName)(uintn_t* variableNameSize, wchar_t* variableName, GUID* vendorGuid);
    status_t (EFIAPI* pSetVariable)(wchar_t* variableName, GUID* vendorGuid, uint32_t attributes, uintn_t dataSize, void* data);
    status_t (EFIAPI* pGetNextHighMonotonicCount)(uint32_t* highCount);
    void (EFIAPI* pResetSystem)(ResetType resetType, status_t resetStatus, uintn_t dataSize, void* resetData);

    // UEFI 2.0
    status_t (EFIAPI* pUpdateCapsule)(CapsuleHeader** capsuleHeaderArray, uintn_t capsuleCount, PhysicalAddress scatterGatherList);
    status_t (EFIAPI* pQueryCapsuleCapabilities)(CapsuleHeader** capsuleHeaderArray, uintn_t capsuleCount, uint64_t* maximumCapsuleSize, ResetType* resetType);
    status_t (EFIAPI* pQueryVariableInfo)(uint32_t attributes, uint64_t* maximumVariableStorageSize, uint64_t* remainingVariableStorageSize, uint64_t* maximumVariableSize);
};



/*
    System Table
*/

struct ConfigurationTable
{
    GUID vendorGuid;
    void* vendorTable;
};


struct SystemTable
{
    static const uint64_t SIGNATURE = 0x5453595320494249ull;

    TableHeader header;

    // EFI 1.0
    wchar_t*                    firmwareVendor;
    uint32_t                    firmwareRevision;
    handle_t                    consoleInHandle;
    SimpleTextInputProtocol*    conIn;
    handle_t                    consoleOutHandle;
    SimpleTextOutputProtocol*   conOut;
    handle_t                    standardErrorHandle;
    SimpleTextOutputProtocol*   stdErr;
    RuntimeServices *           runtimeServices;
    BootServices*               bootServices;
    uintn_t                     numberOfTableEntries;
    ConfigurationTable*         configurationTable;
};



typedef status_t (EFIAPI* ImageEntryPoint)(handle_t imageHandle, SystemTable* systemTable);



/*
    Loaded Image Protocol
*/

struct LoadedImageProtocol
{
    static const GUID guid;

    uint32_t            revision;
    handle_t            parentHandle;
    SystemTable*        systemTable;
    handle_t            deviceHandle;
    DevicePathProtocol* filePath;
    void*               reserved;
    uint32_t            loadOptionsSize;
    void*               loadOptions;
    void*               imageBase;
    uint64_t            imageSize;
    MemoryType          imageCodeType;
    MemoryType          ImageDataType;

    status_t (EFIAPI *pUnload)(handle_t imageHandle);
};



/*
    File Protocol
*/

#define EFI_FILE_PROTOCOL_REVISION_1  0x00010000
#define EFI_FILE_PROTOCOL__REVISION_2 0x00020000

#define EFI_FILE_MODE_READ      0x0000000000000001
#define EFI_FILE_MODE_WRITE     0x0000000000000002
#define EFI_FILE_MODE_CREATE    0x8000000000000000

#define EFI_FILE_READ_ONLY      0x0000000000000001
#define EFI_FILE_HIDDEN         0x0000000000000002
#define EFI_FILE_SYSTEM         0x0000000000000004
#define EFI_FILE_RESERVED       0x0000000000000008
#define EFI_FILE_DIRECTORY      0x0000000000000010
#define EFI_FILE_ARCHIVE        0x0000000000000020
#define EFI_FILE_VALID_ATTR     0x0000000000000037


struct FileIoToken
{
    event_t     event;
    status_t    status;
    uintn_t     bufferSize;
    void*       buffer;
};


struct FileInfo
{
    static const GUID guid;

    uint64_t    signatureze;
    uint64_t    fileSize;
    uint64_t    physicalSize;
    Time        createTime;
    Time        lastAccessTime;
    Time        modificationTime;
    uint64_t    attribute;
    wchar_t     fileName[256];
};



struct FileProtocol
{
    uint64_t revision;

    // Revision 1
    status_t (EFIAPI *pOpen)(FileProtocol* self, FileProtocol** newHandle, const wchar_t* fileName, uint64_t openMode, uint64_t attributes);
    status_t (EFIAPI *pClose)(FileProtocol* self);
    status_t (EFIAPI *pDelete)(FileProtocol* self);
    status_t (EFIAPI *pRead)(FileProtocol* self, uintn_t* bufferSize, void* buffer);
    status_t (EFIAPI *pWrite)(FileProtocol* self, uintn_t* bufferSize, void* buffer);
    status_t (EFIAPI *pGetPosition)(FileProtocol* self, uint64_t* position);
    status_t (EFIAPI *pSetPosition)(FileProtocol* self, uint64_t position);
    status_t (EFIAPI *pGetInfo)(FileProtocol* self, const GUID* informationType, uintn_t* bufferSize, void* buffer);
    status_t (EFIAPI *pSetInfo)(FileProtocol* self, const GUID* informationType, uintn_t bufferSize, void* buffer);
    status_t (EFIAPI *pFlush)(FileProtocol* self);

    // Revision 2
    status_t (EFIAPI *pOpenEx)(FileProtocol* self, FileProtocol** newHandle, wchar_t* fileName, uint64_t openMode, uint64_t attributes, FileIoToken* token);
    status_t (EFIAPI *pReadEx)(FileProtocol* self, FileIoToken* token);
    status_t (EFIAPI *pWriteEx)(FileProtocol* self, FileIoToken* token);
    status_t (EFIAPI *pFlushEx)(FileProtocol* self, FileIoToken* token);


    // Helpers
    status_t Open(FileProtocol** fp, const wchar_t* path)
    {
        return pOpen(this, fp, path, EFI_FILE_MODE_READ, 0);
    }

    status_t Close()
    {
        return pClose(this);
    }

    template<typename T>
    status_t GetInfo(T* info)
    {
        uintn_t size = sizeof(T);
        return pGetInfo(this, &T::guid, &size, info);
    }

    status_t Read(void* buffer, size_t* size)
    {
        return pRead(this, size, buffer);
    }
};


/*
    Simple File System Protocol
*/

struct SimpleFileSystemProtocol
{
    static const GUID guid;

    uint64_t revision;

    status_t (EFIAPI *pOpenVolume)(SimpleFileSystemProtocol* self, FileProtocol** root);

    // Helpers
    status_t OpenVolume(FileProtocol** root)
    {
        return pOpenVolume(this, root);
    }
};


/*
    Load File Protocol
*/

struct LoadFileProtocol
{
    static const GUID guid;

    status_t (EFIAPI *pLoadFile)(LoadFileProtocol* self, DevicePathProtocol* filePath, boolean_t bootPolicy, uintn_t* bufferSize, void* buffer);

    // Helpers
    status_t LoadFile(DevicePathProtocol* path, size_t* bufferSize, void* buffer)
    {
        return pLoadFile(this, path, FALSE, bufferSize, buffer);
    }
};



/*
    EDID Active Protocol
*/

struct EdidActiveProtocol
{
    static const GUID guid;

    uint32_t sizeOfEdid;
    uint8_t* edid;

    // Helpers
    bool Valid() const          { return sizeOfEdid >= 128 && edid; }
};



/*
    Graphics Output Protocol
*/

struct PixelBitmask
{
    uint32_t redMask;
    uint32_t greenMask;
    uint32_t blueMask;
    uint32_t reservedMask;
};


enum GraphicsPixelFormat
{
    PixelRedGreenBlueReserved8BitPerColor,
    PixelBlueGreenRedReserved8BitPerColor,
    PixelBitMask,
    PixelBltOnly,
    PixelFormatMax
};


struct GraphicsOutputModeInformation
{
    uint32_t            version;
    uint32_t            horizontalResolution;
    uint32_t            verticalResolution;
    GraphicsPixelFormat pixelFormat;
    PixelBitmask        pixelInformation;
    uint32_t            pixelsPerScanLine;
};


struct GraphicsOutputProtocolMode
{
    uint32_t                        maxMode;
    uint32_t                        mode;
    GraphicsOutputModeInformation*  info;
    uintn_t                         sizeOfInfo;
    PhysicalAddress                 frameBufferBase;
    uintn_t                         frameBufferSize;
};


struct GraphicsOutputBltPixel
{
    uint8_t blue;
    uint8_t green;
    uint8_t red;
    uint8_t reserved;
};


enum GraphicsOutputBltOperation
{
    EfiBltVideoFill,
    EfiBltVideoToBltBuffer,
    EfiBltBufferToVideo,
    EfiBltVideoToVideo,
    EfiGraphicsOutputBltOperationMax
};



struct GraphicsOutputProtocol
{
    static const GUID guid;

    status_t (EFIAPI *pQueryMode)(GraphicsOutputProtocol* self, uint32_t modeNumber, uintn_t* sizeOfInfo, GraphicsOutputModeInformation** info);
    status_t (EFIAPI *pSetMode)(GraphicsOutputProtocol* self, uint32_t modeNumber);
    status_t (EFIAPI *pBlt)(GraphicsOutputProtocol* self, GraphicsOutputBltPixel* bltBuffer, GraphicsOutputBltOperation bltOperation,
                            uintn_t sourceX, uintn_t sourceY, uintn_t destinationX, uintn_t destinationY,
                            uintn_t width, uintn_t height, uintn_t delta);
    GraphicsOutputProtocolMode* mode;


    // Helpers
    status_t QueryMode(uint32_t mode, GraphicsOutputModeInformation** info)
    {
        uintn_t size = sizeof(GraphicsOutputModeInformation);
        return pQueryMode(this, mode, &size, info);
    }

    status_t SetMode(uint32_t mode)
    {
        return pSetMode(this, mode);
    }
};


} // namespace efi

#endif
