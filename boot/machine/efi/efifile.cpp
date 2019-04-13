/*
    Copyright (c) 2018, Thierry Tremblay
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

#include <rainbow/uefi.h>
#include <Guid/FileInfo.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include "boot.hpp"

static EFI_GUID g_efiFileInfoGuid = EFI_FILE_INFO_ID;
static EFI_GUID g_efiLoadedImageProtocolGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
static EFI_GUID g_efiSimpleFileSystemProtocolGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;

extern EFI_HANDLE g_efiImage;
extern EFI_BOOT_SERVICES* BS;


// Look at this code and tell me EFI isn't insane
EFI_STATUS LoadFile(const wchar_t* path, void*& fileData, size_t& fileSize)
{
    EFI_LOADED_IMAGE_PROTOCOL* image = nullptr;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs = nullptr;
    EFI_FILE_PROTOCOL* fileSystemRoot = nullptr;
    EFI_FILE_PROTOCOL* file = nullptr;
    EFI_FILE_INFO* info = nullptr;
    void* data = nullptr;
    UINTN size;
    EFI_STATUS status;

    // Get access to the boot file system
    status = BS->HandleProtocol(g_efiImage, &g_efiLoadedImageProtocolGuid, (void**)&image);
    if (EFI_ERROR(status) || !image)
        goto error;

    status = BS->HandleProtocol(image->DeviceHandle, &g_efiSimpleFileSystemProtocolGuid, (void**)&fs);
    if (EFI_ERROR(status) || !fs)
        goto error;

    // Open the file system
    status = fs->OpenVolume(fs, &fileSystemRoot);
    if (EFI_ERROR(status))
        goto error;

    // Open the file
    status = fileSystemRoot->Open(fileSystemRoot, &file, const_cast<wchar_t*>(path), EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status))
        goto error;

    // Retrieve the file's size
    size = 0;
    status = file->GetInfo(file, &g_efiFileInfoGuid, &size, nullptr);
    if (EFI_ERROR(status) && status != EFI_BUFFER_TOO_SMALL)
        goto error;

    info = (EFI_FILE_INFO*)malloc(size);
    if (!info)
    {
        status = EFI_OUT_OF_RESOURCES;
        goto error;
    }

    status = file->GetInfo(file, &g_efiFileInfoGuid, &size, info);
    if (EFI_ERROR(status))
        goto error;

    // Allocate memory to hold the file
    data = malloc(info->FileSize);
    if (!data)
    {
        status = EFI_OUT_OF_RESOURCES;
        goto error;
    }

    // Read the file into memory
    size = info->FileSize;
    status = file->Read(file, &size, data);
    if (EFI_ERROR(status) || size != info->FileSize)
        goto error;

    // Return file to caller
    fileData = data;
    fileSize = size;

    goto exit;

error:
    free(data);

exit:
    free(info);
    if (file) file->Close(file);
    if (fileSystemRoot) fileSystemRoot->Close(fileSystemRoot);

    return status;
}