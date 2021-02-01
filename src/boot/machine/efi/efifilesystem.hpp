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

#ifndef _RAINBOW_BOOT_EFIFILESYSTEM_HPP
#define _RAINBOW_BOOT_EFIFILESYSTEM_HPP

#include <cstddef>
#include <rainbow/uefi.h>
#include <Protocol/SimpleFileSystem.h>


class EfiFileHandle
{
public:
    EfiFileHandle()                                 : m_fp(nullptr) {}
    EfiFileHandle(EFI_FILE_PROTOCOL* fp)            : m_fp(fp) {}
    EfiFileHandle(EfiFileHandle&& other)            : m_fp(other.m_fp) { other.m_fp = nullptr; }
    ~EfiFileHandle()                                { if (m_fp) m_fp->Close(m_fp); }
    EfiFileHandle& operator=(EfiFileHandle&& other) { if (m_fp) m_fp->Close(m_fp); m_fp = other.m_fp; other.m_fp = nullptr; return *this; }

    // No copies
    EfiFileHandle(const EfiFileHandle&) = delete;
    EfiFileHandle& operator=(const EfiFileHandle&) = delete;

    explicit operator bool() const noexcept         { return m_fp != nullptr; }

    operator EFI_FILE_PROTOCOL*() const             { return m_fp; }
    EFI_FILE_PROTOCOL* operator->() const           { return m_fp; }

private:
    EFI_FILE_PROTOCOL* m_fp;
};


class EfiFileSystem
{
public:
    EfiFileSystem(EFI_HANDLE hImage, EFI_BOOT_SERVICES* bootServices);

    bool ReadFile(const char16_t* path, void** fileData, size_t* fileSize) const;

private:
    EfiFileHandle m_volume;
};


#endif
