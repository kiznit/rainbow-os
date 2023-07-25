/*
    Copyright (c) 2023, Thierry Tremblay
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

#include <rainbow/uefi.hpp>
#include <type_traits>
#include <unittest.hpp>
#include <utility>

using uintn_t = efi::uintn_t;

template <typename Callable>
union storage
{
    storage() {}
    std::decay_t<Callable> callable;
};

template <int, typename Callable, typename Ret, typename... Args>
auto uefi_mock_(Callable&& c, Ret (*)(Args...) EFIAPI)
{
    static bool used = false;
    static storage<Callable> s;
    using type = decltype(s.callable);

    if (used)
        s.callable.type::~type();
    new (&s.callable) type(std::forward<Callable>(c));
    used = true;

    return [](Args... args) EFIAPI -> Ret { return Ret(s.callable(std::forward<Args>(args)...)); };
}

template <typename Fn, int N = 0, typename Callable>
Fn* uefi_mock(Callable&& c)
{
    return uefi_mock_<N>(std::forward<Callable>(c), (Fn*)nullptr);
}

/*
    efi::BootServices
*/

struct MockBootServicesImpl
{
    MAKE_MOCK5(GetMemoryMap, efi::Status(uintn_t*, efi::MemoryDescriptor*, efi::uintn_t*, efi::uintn_t*, uint32_t*));
    MAKE_MOCK2(ExitBootServices, efi::Status(efi::Handle, uintn_t));
};

class MockBootServices : public efi::BootServices
{
public:
    constexpr MockBootServices()
    {
        GetMemoryMap = uefi_mock<efi::Status EFIAPI(uintn_t*, efi::MemoryDescriptor*, efi::uintn_t*, efi::uintn_t*, uint32_t*)>(
            [this](uintn_t* memoryMapSize, efi::MemoryDescriptor* memoryMap, uintn_t* mapKey, uintn_t* descriptorSize,
                   uint32_t* descriptorVersion) EFIAPI -> efi::Status {
                return mocks.GetMemoryMap(memoryMapSize, memoryMap, mapKey, descriptorSize, descriptorVersion);
            });

        ExitBootServices = uefi_mock<efi::Status EFIAPI(efi::Handle, uintn_t)>(
            [this](efi::Handle imageHandle, uintn_t mapKey)
                EFIAPI -> efi::Status { return mocks.ExitBootServices(imageHandle, mapKey); });
    };

    MockBootServicesImpl mocks;
};

/*
    efi::SimpleTextOutputProtocol
*/

struct MockSimpleTextOutputProtocolImpl
{
    MAKE_MOCK2(OutputString, efi::Status(efi::SimpleTextOutputProtocol*, const char16_t*));
    MAKE_MOCK2(SetAttribute, efi::Status(efi::SimpleTextOutputProtocol*, efi::uintn_t));
};

class MockSimpleTextOutputProtocol : public efi::SimpleTextOutputProtocol
{
public:
    constexpr MockSimpleTextOutputProtocol()
    {
        OutputString = uefi_mock<efi::Status EFIAPI(efi::SimpleTextOutputProtocol*, const char16_t*)>(
            [this](efi::SimpleTextOutputProtocol* self, const char16_t* string)
                EFIAPI -> efi::Status { return mocks.OutputString(self, string); });

        SetAttribute = uefi_mock<efi::Status EFIAPI(efi::SimpleTextOutputProtocol*, efi::uintn_t)>(
            [this](efi::SimpleTextOutputProtocol* self, efi::uintn_t attribute)
                EFIAPI -> efi::Status { return mocks.SetAttribute(self, attribute); });
    }

    MockSimpleTextOutputProtocolImpl mocks;
};
