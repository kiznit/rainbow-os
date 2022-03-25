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

#include "uefi.hpp"
#include "MemoryMap.hpp"
#include "mock.hpp"
#include <unittest.hpp>
#include <vector>

using namespace trompeloeil;

TEST_CASE("ExitBootServices", "[efi]")
{
    MockBootServices bs;
    efi::SystemTable st;
    st.bootServices = &bs;
    g_efiSystemTable = &st;
    g_efiBootServices = &bs;

    SECTION("Normal path")
    {
        REQUIRE_CALL(bs.mocks, GetMemoryMap(_, _, _, _, _))
            .WITH(*(_1) == 0)
            .SIDE_EFFECT(*_1 = 2 * sizeof(efi::MemoryDescriptor))
            .SIDE_EFFECT(*_4 = sizeof(efi::MemoryDescriptor))
            .RETURN(efi::BufferTooSmall);

        REQUIRE_CALL(bs.mocks, GetMemoryMap(_, _, _, _, _))
            .WITH(*(_1) >= 2 * sizeof(efi::MemoryDescriptor))
            .SIDE_EFFECT(*_3 = 0x12345678ul)
            .RETURN(efi::Success);

        REQUIRE_CALL(bs.mocks, ExitBootServices(_, 0x12345678ul)).RETURN(efi::Success);

        const auto memoryMap = ExitBootServices();
        REQUIRE(memoryMap != nullptr);
        REQUIRE(g_efiSystemTable->bootServices == nullptr);
        REQUIRE(g_efiBootServices == nullptr);
    }

    SECTION("GetMemoryMap() failing")
    {
        ALLOW_CALL(bs.mocks, GetMemoryMap(_, _, _, _, _)).RETURN(efi::Unsupported);

        const auto memoryMap = ExitBootServices();
        REQUIRE(!memoryMap);
        REQUIRE(g_efiSystemTable->bootServices != nullptr);
        REQUIRE(g_efiBootServices != nullptr);
    }

    SECTION("ExitBootServices() failing")
    {
        REQUIRE_CALL(bs.mocks, GetMemoryMap(_, _, _, _, _))
            .WITH(*(_1) == 0)
            .SIDE_EFFECT(*_1 = 2 * sizeof(efi::MemoryDescriptor))
            .SIDE_EFFECT(*_4 = sizeof(efi::MemoryDescriptor))
            .RETURN(efi::BufferTooSmall);

        REQUIRE_CALL(bs.mocks, GetMemoryMap(_, _, _, _, _))
            .WITH(*(_1) >= 2 * sizeof(efi::MemoryDescriptor))
            .SIDE_EFFECT(*_3 = 0x12345678ul)
            .RETURN(efi::Success);

        REQUIRE_CALL(bs.mocks, ExitBootServices(_, 0x12345678ul)).RETURN(efi::Unsupported);

        const auto memoryMap = ExitBootServices();
        REQUIRE(!memoryMap);
        REQUIRE(g_efiSystemTable->bootServices != nullptr);
        REQUIRE(g_efiBootServices != nullptr);
    }

    SECTION("Partial shutdown")
    {
        REQUIRE_CALL(bs.mocks, GetMemoryMap(_, _, _, _, _))
            .WITH(*(_1) == 0)
            .SIDE_EFFECT(*_1 = 2 * sizeof(efi::MemoryDescriptor))
            .SIDE_EFFECT(*_4 = sizeof(efi::MemoryDescriptor))
            .RETURN(efi::BufferTooSmall);

        REQUIRE_CALL(bs.mocks, GetMemoryMap(_, _, _, _, _))
            .WITH(*(_1) >= 2 * sizeof(efi::MemoryDescriptor))
            .SIDE_EFFECT(*_3 = 0x12345678ul)
            .RETURN(efi::Success);

        REQUIRE_CALL(bs.mocks, ExitBootServices(_, 0x12345678ul)).RETURN(efi::InvalidParameter);

        REQUIRE_CALL(bs.mocks, GetMemoryMap(_, _, _, _, _))
            .WITH(*(_1) >= 2 * sizeof(efi::MemoryDescriptor))
            .WITH(*(_3) == 0x12345678ull)
            .SIDE_EFFECT(*_3 = 0x87654321ull)
            .RETURN(efi::Success);

        REQUIRE_CALL(bs.mocks, ExitBootServices(_, 0x87654321ull)).RETURN(efi::Success);

        const auto memoryMap = ExitBootServices();
        REQUIRE(memoryMap != nullptr);
        REQUIRE(g_efiSystemTable->bootServices == nullptr);
        REQUIRE(g_efiBootServices == nullptr);
    }
}
