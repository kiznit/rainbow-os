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

#include "MemoryMap.hpp"
#include <iostream>
#include <unittest.hpp>

TEST_CASE("Memory map tracks memory ranges", "[MemoryMap]")
{
    MemoryMap map({{
                       .type = efi::MemoryType::LoaderData,
                       .physicalStart = 0,
                       .virtualStart = 0,
                       .numberOfPages = 1,
                       .attributes = efi::MemoryAttribute::WriteBack,
                   },
                   {
                       .type = efi::MemoryType::Conventional,
                       .physicalStart = 0x1000,
                       .virtualStart = 0,
                       .numberOfPages = 20,
                       .attributes = efi::MemoryAttribute::WriteBack,
                   }},
                  {});

    REQUIRE(map.size() == 2);

    REQUIRE(map[0].type == efi::MemoryType::LoaderData);
    REQUIRE(map[0].attributes == efi::MemoryAttribute::WriteBack);
    REQUIRE(map[0].physicalStart == 0);
    REQUIRE(map[0].numberOfPages == 1);

    REQUIRE(map[1].type == efi::MemoryType::Conventional);
    REQUIRE(map[1].attributes == efi::MemoryAttribute::WriteBack);
    REQUIRE(map[1].physicalStart == 0x1000);
    REQUIRE(map[1].numberOfPages == 20);
}

TEST_CASE("Initialization with custom memory types", "[MemoryMap]")
{
    SECTION("Beginning of range")
    {
        std::vector<efi::MemoryDescriptor> customMemoryTypes;
        customMemoryTypes.emplace_back(efi::MemoryDescriptor{
            .type = efi::MemoryType::KernelCode,
            .physicalStart = 0x1000,
            .virtualStart = 0,
            .numberOfPages = 4,
            .attributes = (efi::MemoryAttribute)0,
        });

        MemoryMap map({{
                          .type = efi::MemoryType::LoaderData,
                          .physicalStart = 0x1000,
                          .virtualStart = 0,
                          .numberOfPages = 10,
                          .attributes = efi::MemoryAttribute::WriteBack,
                      }},
                      customMemoryTypes);

        REQUIRE(map.size() == 2);

        REQUIRE(map[0].type == efi::MemoryType::KernelCode);
        REQUIRE(map[0].attributes == efi::MemoryAttribute::WriteBack);
        REQUIRE(map[0].physicalStart == 0x1000);
        REQUIRE(map[0].numberOfPages == 4);

        REQUIRE(map[1].type == efi::MemoryType::LoaderData);
        REQUIRE(map[1].attributes == efi::MemoryAttribute::WriteBack);
        REQUIRE(map[1].physicalStart == 0x5000);
        REQUIRE(map[1].numberOfPages == 6);
    }

    SECTION("End of range")
    {
        std::vector<efi::MemoryDescriptor> customMemoryTypes;
        customMemoryTypes.emplace_back(efi::MemoryDescriptor{
            .type = efi::MemoryType::KernelCode,
            .physicalStart = 0x7000,
            .virtualStart = 0,
            .numberOfPages = 4,
            .attributes = (efi::MemoryAttribute)0,
        });

        MemoryMap map({{
                          .type = efi::MemoryType::LoaderData,
                          .physicalStart = 0x1000,
                          .virtualStart = 0,
                          .numberOfPages = 10,
                          .attributes = efi::MemoryAttribute::WriteBack,
                      }},
                      customMemoryTypes);

        REQUIRE(map.size() == 2);

        REQUIRE(map[0].type == efi::MemoryType::KernelCode);
        REQUIRE(map[0].attributes == efi::MemoryAttribute::WriteBack);
        REQUIRE(map[0].physicalStart == 0x7000);
        REQUIRE(map[0].numberOfPages == 4);

        REQUIRE(map[1].type == efi::MemoryType::LoaderData);
        REQUIRE(map[1].attributes == efi::MemoryAttribute::WriteBack);
        REQUIRE(map[1].physicalStart == 0x1000);
        REQUIRE(map[1].numberOfPages == 6);
    }

    SECTION("Middle of range")
    {
        std::vector<efi::MemoryDescriptor> customMemoryTypes;
        customMemoryTypes.emplace_back(efi::MemoryDescriptor{
            .type = efi::MemoryType::KernelCode,
            .physicalStart = 0x4000,
            .virtualStart = 0,
            .numberOfPages = 4,
            .attributes = (efi::MemoryAttribute)0,
        });

        MemoryMap map({{
                          .type = efi::MemoryType::LoaderData,
                          .physicalStart = 0x1000,
                          .virtualStart = 0,
                          .numberOfPages = 10,
                          .attributes = efi::MemoryAttribute::WriteBack,
                      }},
                      customMemoryTypes);

        REQUIRE(map.size() == 3);

        REQUIRE(map[0].type == efi::MemoryType::KernelCode);
        REQUIRE(map[0].attributes == efi::MemoryAttribute::WriteBack);
        REQUIRE(map[0].physicalStart == 0x4000);
        REQUIRE(map[0].numberOfPages == 4);

        REQUIRE(map[1].type == efi::MemoryType::LoaderData);
        REQUIRE(map[1].attributes == efi::MemoryAttribute::WriteBack);
        REQUIRE(map[1].physicalStart == 0x1000);
        REQUIRE(map[1].numberOfPages == 3);

        REQUIRE(map[2].type == efi::MemoryType::LoaderData);
        REQUIRE(map[2].attributes == efi::MemoryAttribute::WriteBack);
        REQUIRE(map[2].physicalStart == 0x8000);
        REQUIRE(map[2].numberOfPages == 3);
    }
}

TEST_CASE("Allocate pages", "[MemoryMap]")
{
    SECTION("Allocates a whole descriptor")
    {
        MemoryMap map({{
                           .type = efi::MemoryType::Conventional,
                           .physicalStart = 0x1000,
                           .virtualStart = 0,
                           .numberOfPages = 0x10,
                           .attributes = efi::MemoryAttribute::WriteBack,
                       },
                       {
                           .type = efi::MemoryType::Conventional,
                           .physicalStart = 0x100000,
                           .virtualStart = 0,
                           .numberOfPages = 0x1000,
                           .attributes = efi::MemoryAttribute::WriteBack,
                       }},
                      {});

        const auto memory = map.AllocatePages(0x1000, efi::MemoryType::LoaderData);

        REQUIRE(memory == 0x100000ull);

        REQUIRE(map.size() == 2);
        REQUIRE(map[0].type == efi::MemoryType::Conventional);
        REQUIRE(map[0].physicalStart == 0x1000);
        REQUIRE(map[0].numberOfPages == 0x10);
        REQUIRE(map[1].type == efi::MemoryType::LoaderData);
        REQUIRE(map[1].physicalStart == 0x100000);
        REQUIRE(map[1].numberOfPages == 0x1000);
    }
}
