/*
    Copyright (c) 2024, Thierry Tremblay
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

#include "MemoryMap.hpp"
#include "mock.hpp"
#include <metal/expected.hpp>
#include <metal/shared_ptr.hpp>
#include <metal/vector.hpp>
#include <unittest.hpp>

using namespace trompeloeil;

mtl::expected<mtl::shared_ptr<MemoryMap>, efi::Status> ExitBootServices(efi::Handle hImage, efi::SystemTable* systemTable);

TEST_CASE("ExitBootServices", "[efi]")
{
    MockBootServices bootServices;
    efi::SystemTable systemTable;
    systemTable.bootServices = &bootServices;

    SECTION("Normal path")
    {
        REQUIRE_CALL(bootServices.mocks, GetMemoryMap(_, _, _, _, _))
            .WITH(*(_1) == 0)
            .SIDE_EFFECT(*_1 = 2 * sizeof(efi::MemoryDescriptor))
            .SIDE_EFFECT(*_4 = sizeof(efi::MemoryDescriptor))
            .RETURN(efi::Status::BufferTooSmall);

        REQUIRE_CALL(bootServices.mocks, GetMemoryMap(_, _, _, _, _))
            .WITH(*(_1) >= 2 * sizeof(efi::MemoryDescriptor))
            .SIDE_EFFECT(*_3 = 0x12345678ul)
            .RETURN(efi::Status::Success);

        REQUIRE_CALL(bootServices.mocks, ExitBootServices(_, 0x12345678ul)).RETURN(efi::Status::Success);

        const auto memoryMap = ExitBootServices(nullptr, &systemTable);
        REQUIRE(memoryMap != nullptr);
        REQUIRE(systemTable.bootServices == nullptr);
    }

    SECTION("GetMemoryMap() failing")
    {
        ALLOW_CALL(bootServices.mocks, GetMemoryMap(_, _, _, _, _)).RETURN(efi::Status::Unsupported);

        const auto memoryMap = ExitBootServices(nullptr, &systemTable);
        REQUIRE(!memoryMap);
        REQUIRE(systemTable.bootServices != nullptr);
    }

    SECTION("ExitBootServices() failing")
    {
        REQUIRE_CALL(bootServices.mocks, GetMemoryMap(_, _, _, _, _))
            .WITH(*(_1) == 0)
            .SIDE_EFFECT(*_1 = 2 * sizeof(efi::MemoryDescriptor))
            .SIDE_EFFECT(*_4 = sizeof(efi::MemoryDescriptor))
            .RETURN(efi::Status::BufferTooSmall);

        REQUIRE_CALL(bootServices.mocks, GetMemoryMap(_, _, _, _, _))
            .WITH(*(_1) >= 2 * sizeof(efi::MemoryDescriptor))
            .SIDE_EFFECT(*_3 = 0x12345678ul)
            .RETURN(efi::Status::Success);

        REQUIRE_CALL(bootServices.mocks, ExitBootServices(_, 0x12345678ul)).RETURN(efi::Status::Unsupported);

        const auto memoryMap = ExitBootServices(nullptr, &systemTable);
        REQUIRE(!memoryMap);
        REQUIRE(systemTable.bootServices != nullptr);
    }

    SECTION("Partial shutdown")
    {
        REQUIRE_CALL(bootServices.mocks, GetMemoryMap(_, _, _, _, _))
            .WITH(*(_1) == 0)
            .SIDE_EFFECT(*_1 = 2 * sizeof(efi::MemoryDescriptor))
            .SIDE_EFFECT(*_4 = sizeof(efi::MemoryDescriptor))
            .RETURN(efi::Status::BufferTooSmall);

        REQUIRE_CALL(bootServices.mocks, GetMemoryMap(_, _, _, _, _))
            .WITH(*(_1) >= 2 * sizeof(efi::MemoryDescriptor))
            .SIDE_EFFECT(*_3 = 0x12345678ul)
            .RETURN(efi::Status::Success);

        REQUIRE_CALL(bootServices.mocks, ExitBootServices(_, 0x12345678ul)).RETURN(efi::Status::InvalidParameter);

        REQUIRE_CALL(bootServices.mocks, GetMemoryMap(_, _, _, _, _))
            .WITH(*(_1) >= 2 * sizeof(efi::MemoryDescriptor))
            .WITH(*(_3) == 0x12345678ull)
            .SIDE_EFFECT(*_3 = 0x87654321ull)
            .RETURN(efi::Status::Success);

        REQUIRE_CALL(bootServices.mocks, ExitBootServices(_, 0x87654321ull)).RETURN(efi::Status::Success);

        const auto memoryMap = ExitBootServices(nullptr, &systemTable);
        REQUIRE(memoryMap != nullptr);
        REQUIRE(systemTable.bootServices == nullptr);
    }
}
