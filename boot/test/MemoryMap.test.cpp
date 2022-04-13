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

#include "MemoryMap.hpp"
#include <iostream>
#include <unittest.hpp>

TEST_CASE("Memory map tracks memory ranges", "[MemoryMap]")
{
    MemoryMap map({{.type = MemoryType::Bootloader, .flags = MemoryFlags::WB, .address = 0, .pageCount = 1},
                   {.type = MemoryType::Available, .flags = MemoryFlags::WB, .address = 0x1000, .pageCount = 20}});

    REQUIRE(map.size() == 2);

    REQUIRE(map[0].type == MemoryType::Bootloader);
    REQUIRE(map[0].flags == MemoryFlags::WB);
    REQUIRE(map[0].address == 0);
    REQUIRE(map[0].pageCount == 1);

    REQUIRE(map[1].type == MemoryType::Available);
    REQUIRE(map[1].flags == MemoryFlags::WB);
    REQUIRE(map[1].address == 0x1000);
    REQUIRE(map[1].pageCount == 20);
}

TEST_CASE("Allocate pages", "[MemoryMap]")
{
    MemoryMap map({{.type = MemoryType::Available, .flags = MemoryFlags::WB, .address = 0x1000, .pageCount = 0x10},
                   {.type = MemoryType::Available, .flags = MemoryFlags::WB, .address = 0x100000, .pageCount = 0x1000}});

    SECTION("Allocates from descriptor with highest memory address")
    {
        const auto memory = map.AllocatePages(MemoryType::Bootloader, 1);

        REQUIRE(memory == 0x100000ull);

        REQUIRE(map.size() == 3);

        map.TidyUp();

        REQUIRE(map[0].type == MemoryType::Available);
        REQUIRE(map[0].address == 0x1000);
        REQUIRE(map[0].pageCount == 0x10);
        REQUIRE(map[1].type == MemoryType::Bootloader);
        REQUIRE(map[1].address == 0x100000);
        REQUIRE(map[1].pageCount == 1);
        REQUIRE(map[2].type == MemoryType::Available);
        REQUIRE(map[2].address == 0x101000);
        REQUIRE(map[2].pageCount == 0xFFF);
    }

    SECTION("Allocates a whole descriptor")
    {
        const auto memory = map.AllocatePages(MemoryType::Bootloader, 0x1000);

        REQUIRE(memory == 0x100000ull);

        REQUIRE(map.size() == 2);
        REQUIRE(map[0].type == MemoryType::Available);
        REQUIRE(map[0].address == 0x1000);
        REQUIRE(map[0].pageCount == 0x10);
        REQUIRE(map[1].type == MemoryType::Bootloader);
        REQUIRE(map[1].address == 0x100000);
        REQUIRE(map[1].pageCount == 0x1000);
    }
}

TEST_CASE("MemoryMap handles overlaps correctly", "[MemoryMap]")
{
    MemoryMap map({{.type = MemoryType::Available, .flags = MemoryFlags::WB, .address = 0x102000, .pageCount = 8}});

    SECTION("overlap at start")
    {
        map.SetMemoryRange(0x100000, 4, MemoryType::Reserved, MemoryFlags::WB);

        REQUIRE(map.size() == 2);

        REQUIRE(map[0].type == MemoryType::Reserved);
        REQUIRE(map[0].flags == MemoryFlags::WB);
        REQUIRE(map[0].address == 0x100000);
        REQUIRE(map[0].pageCount == 4);

        REQUIRE(map[1].type == MemoryType::Available);
        REQUIRE(map[1].flags == MemoryFlags::WB);
        REQUIRE(map[1].address == 0x104000);
        REQUIRE(map[1].pageCount == 6);
    }

    SECTION("overlap at end")
    {
        map.SetMemoryRange(0x108000, 4, MemoryType::Reserved, MemoryFlags::WB);

        REQUIRE(map.size() == 2);

        REQUIRE(map[0].type == MemoryType::Reserved);
        REQUIRE(map[0].flags == MemoryFlags::WB);
        REQUIRE(map[0].address == 0x108000);
        REQUIRE(map[0].pageCount == 4);

        REQUIRE(map[1].type == MemoryType::Available);
        REQUIRE(map[1].flags == MemoryFlags::WB);
        REQUIRE(map[1].address == 0x102000);
        REQUIRE(map[1].pageCount == 6);
    }

    SECTION("overlap in middle")
    {
        map.SetMemoryRange(0x104000, 3, MemoryType::Reserved, MemoryFlags::WB);

        REQUIRE(map.size() == 3);

        REQUIRE(map[0].type == MemoryType::Reserved);
        REQUIRE(map[0].flags == MemoryFlags::WB);
        REQUIRE(map[0].address == 0x104000);
        REQUIRE(map[0].pageCount == 3);

        REQUIRE(map[1].type == MemoryType::Available);
        REQUIRE(map[1].flags == MemoryFlags::WB);
        REQUIRE(map[1].address == 0x102000);
        REQUIRE(map[1].pageCount == 2);

        REQUIRE(map[2].type == MemoryType::Available);
        REQUIRE(map[2].flags == MemoryFlags::WB);
        REQUIRE(map[2].address == 0x107000);
        REQUIRE(map[2].pageCount == 3);
    }

    SECTION("overlap both ends")
    {
        map.SetMemoryRange(0x101000, 10, MemoryType::Reserved, MemoryFlags::WB);

        REQUIRE(map.size() == 1);

        REQUIRE(map[0].type == MemoryType::Reserved);
        REQUIRE(map[0].flags == MemoryFlags::WB);
        REQUIRE(map[0].address == 0x101000);
        REQUIRE(map[0].pageCount == 10);

        map.SetMemoryRange(0x100000, 16, MemoryType::Available, MemoryFlags::WB);

        REQUIRE(map.size() == 3);

        REQUIRE(map[0].type == MemoryType::Reserved);
        REQUIRE(map[0].flags == MemoryFlags::WB);
        REQUIRE(map[0].address == 0x101000);
        REQUIRE(map[0].pageCount == 10);

        REQUIRE(map[1].type == MemoryType::Available);
        REQUIRE(map[1].flags == MemoryFlags::WB);
        REQUIRE(map[1].address == 0x100000);
        REQUIRE(map[1].pageCount == 1);

        REQUIRE(map[2].type == MemoryType::Available);
        REQUIRE(map[2].flags == MemoryFlags::WB);
        REQUIRE(map[2].address == 0x10B000);
        REQUIRE(map[2].pageCount == 5);
    }
}
