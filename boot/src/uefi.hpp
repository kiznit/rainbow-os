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

#pragma once

#include "MemoryMap.hpp"
#include <expected>
#include <metal/arch.hpp>
#include <rainbow/uefi.hpp>
#include <rainbow/uefi/filesystem.hpp>

// Bootloader entry point (equivalent to main() in standard C++).
// Cannot use "main" as the function name as this causes problems with mingw.
efi::Status efi_main(efi::SystemTable* systemTable);

std::expected<mtl::PhysicalAddress, efi::Status> AllocatePages(size_t pageCount);

std::expected<std::shared_ptr<MemoryMap>, efi::Status> ExitBootServices();

std::expected<char16_t, efi::Status> GetChar();

std::expected<efi::FileProtocol*, efi::Status> InitializeFileSystem();

void SetupConsoleLogging();
std::expected<void, efi::Status> SetupFileLogging(efi::FileProtocol* fileSystem);
