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

#include "base.hpp"

namespace efi
{
    constexpr Guid FileInfoGuid{
        0x9576e92, 0x6d3f, 0x11d2, {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};
    constexpr Guid SimpleFileSystemProtocolGuid{
        0x964e5b22, 0x6459, 0x11d2, {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};

    struct FileProtocol;

    struct SimpleFileSystemProtocol
    {
        uint64_t revision;
        Status(EFIAPI* OpenVolume)(SimpleFileSystemProtocol* self, FileProtocol** root);
    };

    enum OpenMode : uint64_t
    {
        FileModeRead = 0x0000000000000001ull,
        FileModeWrite = 0x0000000000000003ull,
        FileModeCreate = 0x8000000000000003ull,
    };

    enum FileAttribute : uint64_t
    {
        FileReadOnly = 0x0000000000000001ull,
        FileHidden = 0x0000000000000002ull,
        FileSystem = 0x0000000000000004ull,
        FileReserved = 0x0000000000000008ull,
        FileDirectory = 0x0000000000000010ull,
        FileArchive = 0x0000000000000020ull,
    };

    struct FileInfo
    {
        uint64_t size;
        uint64_t fileSize;
        uint64_t physicalSize;
        Time createTime;
        Time lastAccessTime;
        Time modificationTime;
        uint64_t attribute;
        char16_t fileName[1];
    };

    struct FileIoToken
    {
        Event event;
        Status status;
        uintn_t bufferSize;
        void* buffer;
    };

    struct FileProtocol
    {
        uint64_t revision;

        Status(EFIAPI* Open)(FileProtocol* self, FileProtocol** newHandle, const char16_t* fileName,
                             OpenMode openMode, uintn_t attributes);
        Status(EFIAPI* Close)(FileProtocol* self);
        Status(EFIAPI* Delete)(FileProtocol* self);
        Status(EFIAPI* Read)(FileProtocol* self, uintn_t* bufferSize, void* buffer);
        Status(EFIAPI* Write)(FileProtocol* self, uintn_t* bufferSize, const void* buffer);
        Status(EFIAPI* GetPosition)(FileProtocol* self, uint64_t* position);
        Status(EFIAPI* SetPosition)(FileProtocol* self, uint64_t position);
        Status(EFIAPI* GetInfo)(FileProtocol* self, const Guid* informationType,
                                uintn_t* bufferSize, void* buffer);
        Status(EFIAPI* SetInfo)(FileProtocol* self, const Guid* informationType, uintn_t bufferSize,
                                const void* buffer);
        Status(EFIAPI* Flush)(FileProtocol* self);

        Status(EFIAPI* OpenEx)(FileProtocol* self, FileProtocol** newHandle,
                               const char16_t* fileName, OpenMode openMode, uintn_t attributes,
                               FileIoToken* token);
        Status(EFIAPI* ReadEx)(FileProtocol* self, FileIoToken* token);
        Status(EFIAPI* WriteEx)(FileProtocol* self, FileIoToken* token);
        Status(EFIAPI* FlushEx)(FileProtocol* self, FileIoToken* token);
    };

} // namespace efi
