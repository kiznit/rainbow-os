
/*
    Copyright (c) 2015, Thierry Tremblay
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

#include "efi.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


namespace efi {

// Sanity checks
static_assert(sizeof(wchar_t) == 2, "wchar_t must be 2 bytes wide");
static_assert(sizeof(Time) == 16, "Time structure isn't packed properly");

static_assert(sizeof(FileInfo) == 4*sizeof(uint64_t) + 3*sizeof(Time) + 256*sizeof(wchar_t), "FileInfo structure isn't packed properly");


const GUID DevicePathProtocol::guid         = { 0x09576e91, 0x6d3f, 0x11d2, { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b }};
const GUID EdidActiveProtocol::guid         = { 0xbd8c1056, 0x9f36, 0x44ec, { 0x92, 0xa8, 0xa6, 0x33, 0x7f, 0x81, 0x79, 0x86 }};
const GUID FileInfo::guid                   = { 0x09576e92, 0x6d3f, 0x11d2, { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b }};
const GUID GraphicsOutputProtocol::guid     = { 0x9042a9de, 0x23dc, 0x4a38, { 0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a }};
const GUID LoadedImageProtocol::guid        = { 0x5B1B31A1, 0x9562, 0x11d2, { 0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B }};
const GUID LoadFileProtocol::guid           = { 0x56EC3091, 0x954C, 0x11d2, { 0x8e, 0x3f, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b }};
const GUID SimpleFileSystemProtocol::guid   = { 0x964e5b22, 0x6459, 0x11d2, { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b }};
const GUID SimpleTextInputProtocol::guid    = { 0x387477c1, 0x69c7, 0x11d2, { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b }};
const GUID SimpleTextOutputProtocol::guid   = { 0x387477c2, 0x69c7, 0x11d2, { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b }};



/*
    Boot Services
*/


MemoryDescriptor* BootServices::GetMemoryMap(uintn_t* descriptorCount, uintn_t* descriptorSize, uint32_t* descriptorVersion, uintn_t* mapKey)
{
    *descriptorCount = 0;
    *descriptorSize = 0;
    *descriptorVersion = 0;
    *mapKey = 0;

    status_t status;
    uintn_t size = 0;

    status = pGetMemoryMap(&size, NULL, mapKey, descriptorSize, descriptorVersion);
    if (status != EFI_BUFFER_TOO_SMALL)
        return NULL;

    MemoryDescriptor* buffer = NULL;

    while (status == EFI_BUFFER_TOO_SMALL)
    {
        if (buffer)
        {
            free(buffer);
            buffer = NULL;
        }

        buffer = (MemoryDescriptor*) malloc(size);
        status = pGetMemoryMap(&size, buffer, mapKey, descriptorSize, descriptorVersion);
    }

    if (EFI_ERROR(status))
    {
        free(buffer);
        return NULL;
    }

    *descriptorCount = size / *descriptorSize;

    return buffer;
}



status_t BootServices::LocateHandle(const GUID* protocol, size_t* handleCount, handle_t** handles)
{
    if (header.revision < EFI_REVISION_1_10)
    {
        size_t size = 0;
        handle_t* buffer = NULL;
        status_t status = pLocateHandle(ByProtocol, protocol, NULL, &size, buffer);

        if (status == EFI_BUFFER_TOO_SMALL)
        {
            buffer = (handle_t*)Allocate(size);
            status = pLocateHandle(ByProtocol, protocol, NULL, &size, buffer);
        }

        *handleCount = size / sizeof(handle_t);
        *handles = (handle_t*)buffer;

        return status;
    }
    else
    {
        return pLocateHandleBuffer(ByProtocol, protocol, NULL, handleCount, handles);
    }
}



} // namespace efi
