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

#include <catch2/catch.hpp>
#include "memory.hpp"


TEST_CASE("MemoryMap rounds available memory down", "[MemoryMap]")
{
    MemoryMap map;
    map.AddBytes(MemoryType::Available, MemoryFlags::WB, 0x100, 0x4000);
    REQUIRE(map.size() == 1);
    REQUIRE(map[0].start() == 0x1000);
    REQUIRE(map[0].end() == 0x4000);
}


TEST_CASE("MemoryMap rounds reserved memory up", "[MemoryMap]")
{
    MemoryMap map;
    map.AddBytes(MemoryType::Reserved, MemoryFlags::WB, 0x100, 0x4000);
    REQUIRE(map.size() == 1);
    REQUIRE(map[0].start() == 0x0000);
    REQUIRE(map[0].end() == 0x5000);
}


TEST_CASE("MemoryMap handles overlaps correctly", "[MemoryMap]")
{
    MemoryMap map;
    map.AddBytes(MemoryType::Available, MemoryFlags::WB, 0, 0xA0000);

    SECTION("overlap at start")
    {
        map.AddBytes(MemoryType::Reserved, MemoryFlags::WB, 0, 0x600);

        REQUIRE(map.size() == 2);

        REQUIRE(map[0].type == MemoryType::Reserved);
        REQUIRE(map[0].start() == 0);
        REQUIRE(map[0].end() == 0x1000);

        REQUIRE(map[1].type == MemoryType::Available);
        REQUIRE(map[1].start() == 0x1000);
        REQUIRE(map[1].end() == 0xA0000);
    }

    SECTION("overlap at end")
    {
        map.AddBytes(MemoryType::Reserved, MemoryFlags::WB, 0x9E800, 0x1800);

        REQUIRE(map.size() == 2);

        REQUIRE(map[1].type == MemoryType::Available);
        REQUIRE(map[1].start() == 0);
        REQUIRE(map[1].end() == 0x9E000);

        REQUIRE(map[0].type == MemoryType::Reserved);
        REQUIRE(map[0].start() == 0x9E000);
        REQUIRE(map[0].end() == 0xA0000);
    }

    SECTION("overlap in middle")
    {
        map.AddBytes(MemoryType::Reserved, MemoryFlags::WB, 0x20000, 0x10000);

        REQUIRE(map.size() == 3);

        REQUIRE(map[0].type == MemoryType::Reserved);
        REQUIRE(map[0].start() == 0x20000);
        REQUIRE(map[0].end() == 0x30000);

        REQUIRE(map[1].type == MemoryType::Available);
        REQUIRE(map[1].start() == 0);
        REQUIRE(map[1].end() == 0x20000);

        REQUIRE(map[2].type == MemoryType::Available);
        REQUIRE(map[2].start() == 0x30000);
        REQUIRE(map[2].end() == 0xA0000);
    }
}
