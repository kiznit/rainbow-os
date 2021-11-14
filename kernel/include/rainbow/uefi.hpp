/*
    Copyright (c) 2021, Thierry Tremblay
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

#pragma once

#include "uefi/base.hpp"

namespace efi
{
    struct InputKey
    {
        uint16_t scanCode;
        char16_t unicodeChar;
    };

    static_assert(sizeof(efi::InputKey) == 4);

    class SimpleTextInputProtocol
    {
    public:
        Status(EFIAPI* Reset)(SimpleTextInputProtocol* self, bool extendedVerification);
        Status(EFIAPI* ReadKeyStroke)(SimpleTextInputProtocol* self, InputKey* key);
        Event waitForKey;
    };

    static_assert(sizeof(efi::SimpleTextInputProtocol) == 3 * sizeof(void*));

    struct SimpleTextOutputMode
    {
        int32_t maxMode;
        int32_t mode;
        int32_t attribute;
        int32_t cursorColumn;
        int32_t cursorRow;
        bool cursorVisible;
    };

    static_assert(sizeof(efi::SimpleTextOutputMode) == 24);

    enum TextAttribute : uintn_t
    {
        // Text colours
        Black = 0,
        Blue,
        Green,
        Cyan,
        Red,
        Magenta,
        Brown,
        LightGray,
        DarkGray,
        LightBlue,
        LightGreen,
        LightCyan,
        LightRed,
        LightMagenta,
        Yellow,
        White,

        // Background colours
        BackgroundBlack = 0x00,
        BackgroundBlue = 0x10,
        BackgroundGreen = 0x20,
        BackgroundCyan = 0x30,
        BackgroundRed = 0x40,
        BackgroundMagenta = 0x50,
        BackgroundBrown = 0x60,
        BackgroundLightGray = 0x70,

        // Others
        Bright = 0x08,
        Wide = 0x80,
    };

    class SimpleTextOutputProtocol
    {
    public:
        Status(EFIAPI* Reset)(SimpleTextOutputProtocol* self, bool extendedVerification);
        Status(EFIAPI* OutputString)(SimpleTextOutputProtocol* self, const char16_t* string);
        Status(EFIAPI* TestString)(const SimpleTextOutputProtocol* self, const char16_t* string);
        Status(EFIAPI* QueryMode)(const SimpleTextOutputProtocol* self, uintn_t modeNumber,
                                  uintn_t* columns, uintn_t* rows);
        Status(EFIAPI* SetMode)(SimpleTextOutputProtocol* self, uintn_t modeNumber);
        Status(EFIAPI* SetAttribute)(SimpleTextOutputProtocol* self, TextAttribute attribute);
        Status(EFIAPI* ClearScreen)(SimpleTextOutputProtocol* self);
        Status(EFIAPI* SetCursorPosition)(SimpleTextOutputProtocol* self, uintn_t column,
                                          uintn_t row);
        Status(EFIAPI* EnableCursor)(SimpleTextOutputProtocol* self, bool visible);
        const SimpleTextOutputMode* mode;
    };

    static_assert(sizeof(efi::SimpleTextOutputProtocol) == 10 * sizeof(void*));

    struct MemoryDescriptor
    {
        uint32_t type;
        uint32_t padding;
        PhysicalAddress physicalStart;
        VirtualAddress virtualStart;
        uint64_t numberOfPages;
        uint64_t attribute;
    };

    static_assert(sizeof(efi::MemoryDescriptor) == 40);

    enum ResetType
    {
        EfiResetCold,
        EfiResetWarm,
        EfiResetShutdown,
        EfiResetPlatformSpecific
    };

    struct CapsuleHeader
    {
        Guid CapsuleGuid;
        uint32_t HeaderSize;
        uint32_t Flags;
        uint32_t CapsuleImageSize;
    };

    static_assert(sizeof(efi::CapsuleHeader) == 28);

    class RuntimeServices : public TableHeader
    {
    public:
        // Time Services
        Status(EFIAPI* GetTime)(Time* time, TimeCapabilities* capabilities);
        Status(EFIAPI* SetTime)(const Time* time);
        Status(EFIAPI* GetWakeupTime)(bool* enabled, bool* pending, Time* time);
        Status(EFIAPI* SetWakeupTime)(bool enabled, const Time* time);

        // Virtual Memory Services
        Status(EFIAPI* SetVirtualAddressMap)(uintn_t memoryMapSize, uintn_t descriptorSize,
                                             uint32_t descriptorVersion,
                                             MemoryDescriptor* virtualMap);
        Status(EFIAPI* ConvertPointer)(uintn_t debugDisposition, void** address);

        // Variable Services
        Status(EFIAPI* GetVariable)(const char16_t* variableName, const Guid* vendorGuid,
                                    uint32_t* attributes, uintn_t* dataSize, void* data);
        Status(EFIAPI* GetNextVariableName)(uintn_t* variableNameSize, char16_t* variableName,
                                            Guid* vendorGuid);
        Status(EFIAPI* SetVariable)(const char16_t* variableName, const Guid* vendorGuid,
                                    uint32_t attributes, uintn_t dataSize, const void* data);

        // Miscellaneous Services
        Status(EFIAPI* GetNextHighMonotonicCount)(uint32_t* highCount);
        void(EFIAPI* ResetSystem)(ResetType resetType, Status resetStatus, uintn_t dataSize,
                                  const void* resetData);

        // UEFI 2.0 Capsule Services
        Status(EFIAPI* UpdateCapsule)(const CapsuleHeader** capsuleHeaderArray,
                                      uintn_t capsuleCount, PhysicalAddress scatterGatherList);
        Status(EFIAPI* QueryCapsuleCapabilities)(const CapsuleHeader** capsuleHeaderArray,
                                                 uintn_t capsuleCount, uint64_t* maximumCapsuleSize,
                                                 ResetType* resetType);

        // Miscellaneous UEFI 2.0 Service
        Status(EFIAPI* QueryVariableInfo)(uint32_t attributes, uint64_t* maximumVariableStorageSize,
                                          uint64_t* remainingVariableStorageSize,
                                          uint64_t* maximumVariableSize);
    };

    static_assert(sizeof(efi::RuntimeServices) == sizeof(TableHeader) + 14 * sizeof(void*));

    enum LocateSearchType
    {
        AllHandles,
        ByRegisterNotify,
        ByProtocol,
    };

    enum Tpl : uintn_t
    {
        Application = 4,
        Callback = 8,
        Notify = 16,
        HighLevel = 31
    };

    enum AllocateType
    {
        AllocateAnyPages,
        AllocateMaxAddress,
        AllocateAddress,
        MaxAllocateType
    };

    enum MemoryAttribute
    {
        MemoryUC = 0x0000000000000001ull,
        MemoryWC = 0x0000000000000002ull,
        MemoryWT = 0x0000000000000004ull,
        MemoryWB = 0x0000000000000008ull,
        MemoryUCE = 0x0000000000000010ull,

        MemoryWP = 0x0000000000001000ull,
        MemoryRP = 0x0000000000002000ull,
        MemoryXP = 0x0000000000004000ull,
        MemoryRO = 0x0000000000020000ull,

        MemoryNV = 0x0000000000008000ull,
        MemoryMoreReliable = 0x0000000000010000ull,
        MemoryRuntime = 0x8000000000000000ull
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

    static constexpr auto PageSize = 4096ull;

    using EventNotify = void(Event event, void* context);

    enum TimerDelay
    {
        TimerCancel,
        TimerPeriodic,
        TimerRelative
    };

    enum InterfaceType
    {
        NativeInterface
    };

    constexpr Guid DevicePathProtocolGuid{
        0x9576e91, 0x6d3f, 0x11d2, {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};

    struct DevicePathProtocol
    {
        uint8_t type;
        uint8_t subType;
        uint8_t length[2];
    };

    static_assert(sizeof(efi::DevicePathProtocol) == 4);

    struct OpenProtocolInformationEntry
    {
        Handle agentHandle;
        Handle controllerHandle;
        uint32_t attributes;
        uint32_t openCount;
    };

    static_assert(sizeof(efi::OpenProtocolInformationEntry) == 8 + 2 * sizeof(void*));

    class BootServices : public TableHeader
    {
    public:
        // Task Priority Services
        Status(EFIAPI* RaiseTPL)(Tpl newTpl);
        Status(EFIAPI* RestoreTPL)(Tpl oldTpl);

        // Memory Services
        Status(EFIAPI* AllocatePages)(AllocateType type, MemoryType memoryType, uintn_t pages,
                                      PhysicalAddress* memory);
        Status(EFIAPI* FreePages)(PhysicalAddress memory, uintn_t pages);
        Status(EFIAPI* GetMemoryMap)(uintn_t* memoryMapSize, MemoryDescriptor* memoryMap,
                                     uintn_t* mapKey, uintn_t* descriptorSize,
                                     uint32_t* descriptorVersion);
        Status(EFIAPI* AllocatePool)(MemoryType poolType, uintn_t size, void** buffer);
        Status(EFIAPI* FreePool)(const void* buffer);

        // Event & Timer Services
        Status(EFIAPI* CreateEvent)(uint32_t type, Tpl notifyTpl, EventNotify notifyFunction,
                                    const void* notifyContext, Event* event);
        Status(EFIAPI* SetTimer)(Event event, TimerDelay type, uint64_t triggerTime);

        Status(EFIAPI* WaitForEvent)(const uintn_t* event, uintn_t* index);
        Status(EFIAPI* SignalEvent)(Event event);
        Status(EFIAPI* CloseEvent)(Event event);
        Status(EFIAPI* CheckEvent)(Event event);

        // Protocol Handler Services
        Status(EFIAPI* InstallProtocolInterface)(Handle* handle, const Guid* protocol,
                                                 InterfaceType interfaceType, void* interface);
        Status(EFIAPI* ReinstallProtocolInterface)(Handle handle, const Guid* protocol,
                                                   void* oldInterface, void* newInterface);
        Status(EFIAPI* UninstallProtocolInterface)(Handle handle, const Guid* protocol,
                                                   void* interface);
        Status(EFIAPI* HandleProtocol)(Handle handle, const Guid* protocol, void** interface);
        const void* reserved;
        Status(EFIAPI* RegisterProtocolNotify)(const Guid* protocol, Event event,
                                               void** registration);
        Status(EFIAPI* LocateHandle)(LocateSearchType searchType, const Guid* protocol,
                                     const void* searchKey, uintn_t* bufferSize, Handle* buffer);
        Status(EFIAPI* LocateDevicePath)(const Guid* protocol, DevicePathProtocol** devicePath,
                                         Handle* device);
        Status(EFIAPI* InstallConfigurationTable)(const Guid* guid, const void* table);

        // Image Services
        Status(EFIAPI* LoadImage)(bool bootPolicy, Handle parentImageHandle,
                                  const DevicePathProtocol* devicePath, const void* sourceBuffer,
                                  uintn_t sourceSize, Handle* imageHandle);
        Status(EFIAPI* StartImage)(Handle imageHandle, uintn_t* exitDataSize, char16_t** exitData);
        Status(EFIAPI* Exit)(Handle imageHandle, Status exitStatus, uintn_t exitDataSize,
                             const char16_t* exitData);
        Status(EFIAPI* UnloadImage)(Handle imageHandle);
        Status(EFIAPI* ExitBootServices)(Handle imageHandle, uintn_t mapKey);

        // Miscellaneous Services
        Status(EFIAPI* GetNextMonotonicCount)(uint64_t* count);
        Status(EFIAPI* Stall)(uintn_t microseconds);
        Status(EFIAPI* SetWatchdogTimer)(uintn_t tmeout, uint64_t watchdogCode, uintn_t dataSize,
                                         const char16_t* watchdogData);

        // DriverSupport Services
        Status(EFIAPI* ConnectController)(Handle controllerHandle, const Handle* driverImageHandle,
                                          const DevicePathProtocol* remainingDevicePath,
                                          bool recursive);
        Status(EFIAPI* DisconnectController)(Handle controllerHandle, Handle driverImageHandle,
                                             Handle childHandle);

        // Open and Close Protocol Services
        Status(EFIAPI* OpenProtocol)(Handle handle, const Guid* protocol, void** interface,
                                     Handle agentHandle, Handle controllerHandle,
                                     uint32_t attributes);
        Status(EFIAPI* CloseProtocol)(Handle handle, const Guid* protocol, Handle agentHandle,
                                      Handle controllerHandle);
        Status(EFIAPI* OpenProtocolInformation)(Handle handle, const Guid* protocol,
                                                OpenProtocolInformationEntry** entryBuffer,
                                                uintn_t* entryCount);

        // Library Services
        Status(EFIAPI* ProtocolsPerHandle)(Handle handle, Guid*** protocolBuffer,
                                           uintn_t* protocolBufferCount);
        Status(EFIAPI* LocateHandleBuffer)(LocateSearchType searchType, const Guid* protocol,
                                           void* searchKey, uintn_t* noHandles, Handle** buffer);
        Status(EFIAPI* LocateProtocol)(const Guid* protocol, const void* registration,
                                       void** interface);
        Status(EFIAPI* InstallMultipleProtocolInterfaces)(Handle* handle, ...);
        Status(EFIAPI* UninstallMultipleProtocolInterfaces)(Handle handle, ...);

        // 32-bit CRC Services
        Status(EFIAPI* CalculateCrc32)(const void* data, uintn_t dataSize, uint32_t* crc32);

        // Miscellaneous Services
        Status(EFIAPI* CopyMem)(void* destination, const void* source, uintn_t length);
        Status(EFIAPI* SetMem)(void* buffer, uintn_t size, uint8_t value);
        Status(EFIAPI* CreateEventEx)(uint32_t type, Tpl notifyTpl, EventNotify notifyFunction,
                                      const void* notifyContext, const Guid* eventGroup,
                                      Event* event);
    };

    static_assert(sizeof(efi::BootServices) == sizeof(TableHeader) + 44 * sizeof(void*));

    struct ConfigurationTable
    {
        Guid vendorGuid;
        const void* vendorTable;
    };

    static_assert(sizeof(efi::ConfigurationTable) == 16 + sizeof(void*));

    struct SystemTable : TableHeader
    {
        const char16_t* firmwareVendor;
        uint32_t firmwareRevision;
        Handle consoleInHandle;
        SimpleTextInputProtocol* conIn;
        Handle consoleOutHandle;
        SimpleTextOutputProtocol* conOut;
        Handle standardErrorHandle;
        SimpleTextOutputProtocol* stdErr;
        RuntimeServices* runtimeServices;
        BootServices* bootServices;
        uintn_t numberOfTableEntries;
        ConfigurationTable* configurationTable;
    };

    static_assert(sizeof(efi::SystemTable) == sizeof(TableHeader) + 12 * sizeof(void*));

} // namespace efi
