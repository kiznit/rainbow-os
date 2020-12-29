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

#include "efifilesystem.hpp"
#include <cstdlib>
#include <Guid/FileInfo.h>
#include <Protocol/LoadedImage.h>
#include "boot.hpp"

static EFI_GUID g_efiFileInfoGuid = EFI_FILE_INFO_ID;
static EFI_GUID g_efiLoadedImageProtocolGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
static EFI_GUID g_efiSimpleFileSystemProtocolGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;


// Look at this code and tell me EFI isn't insane

EfiFileSystem::EfiFileSystem(EFI_HANDLE hImage, EFI_BOOT_SERVICES* bootServices)
:   m_volume(nullptr)
{
    EFI_LOADED_IMAGE_PROTOCOL* image = nullptr;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs = nullptr;
    EFI_FILE_PROTOCOL* volume = nullptr;
    EFI_STATUS status;

    // Get access to the boot file system
    status = bootServices->HandleProtocol(hImage, &g_efiLoadedImageProtocolGuid, (void**)&image);
    if (EFI_ERROR(status))
        return;

    status = bootServices->HandleProtocol(image->DeviceHandle, &g_efiSimpleFileSystemProtocolGuid, (void**)&fs);
    if (EFI_ERROR(status))
        return;

    // Open the file system
    status = fs->OpenVolume(fs, &volume);
    if (EFI_ERROR(status))
        return;

    m_volume = volume;
}



EfiFileSystem::~EfiFileSystem()
{
    if (m_volume)
    {
        m_volume->Close(m_volume);
    }
}


bool EfiFileSystem::ReadFile(const wchar_t* path, void** fileData, size_t* fileSize) const
{
    if (!m_volume)
        return false;

    EFI_FILE_PROTOCOL* file = nullptr;
    EFI_FILE_INFO* info = nullptr;
    void* data = nullptr;
    int pageCount = 0;
    UINTN size;
    EFI_STATUS status;

    // Open the file
    status = m_volume->Open(m_volume, &file, const_cast<wchar_t*>(path), EFI_FILE_MODE_READ, 0);
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
    // We use pages because we want ELF files to be page-aligned
    pageCount = align_up(info->FileSize, MEMORY_PAGE_SIZE) >> MEMORY_PAGE_SHIFT;
    data = g_bootServices->AllocatePages(pageCount);
    if (!data)
    {
        status = EFI_OUT_OF_RESOURCES;
        goto error;
    }

    // Read the file into memory
    size = info->FileSize;
    status = file->Read(file, &size, data);
    if (EFI_ERROR(status))
        goto error;

    // Return file to caller
    *fileData = data;
    *fileSize = size;

    goto exit;

error:
    //TODO: ?
    //FreePages(data, pageCount);

exit:
    free(info);
    if (file) file->Close(file);

    return EFI_ERROR(status) ? false : true;
}
