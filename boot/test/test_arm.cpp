/*
    Copyright (c) 2017, Thierry Tremblay
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

#include <gtest/gtest.h>
#include <fstream>
#include <iterator>
#include <vector>
#include <rainbow/boot.hpp>
#include "../memory.hpp"
#include "../raspi/arm.hpp"



TEST(ARM, Atags)
{
    std::vector<char> atags;
    std::ifstream file("data/raspi3_atags.bin", std::ios::binary);
    atags.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    ASSERT_FALSE(atags.empty());

    BootInfo info;
    MemoryMap memory;
    ProcessBootParameters(&*atags.begin(), &info, &memory);

    EXPECT_EQ(info.initrdAddress, 0x10000000);
    EXPECT_EQ(info.initrdSize, 70436);

    EXPECT_EQ(memory.size(), 1);
    EXPECT_EQ(memory[0].type, MemoryType_Available);
    EXPECT_EQ(memory[0].flags, 0);
    EXPECT_EQ(memory[0].address, 0);
    EXPECT_EQ(memory[0].numberOfPages, 0x3b000000 >> 12);
}



TEST(ARM, DeviceTree)
{
    std::vector<char> fdt;
    std::ifstream file("data/raspi3_fdt.dtb", std::ios::binary);
    fdt.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    ASSERT_FALSE(fdt.empty());

    BootInfo info;
    MemoryMap memory;
    ProcessBootParameters(&*fdt.begin(), &info, &memory);

    EXPECT_EQ(info.initrdAddress, 0x10000000);
    EXPECT_EQ(info.initrdSize, 70436);

    EXPECT_EQ(memory.size(), 1);
    EXPECT_EQ(memory[0].type, MemoryType_Reserved);
    EXPECT_EQ(memory[0].flags, 0);
    EXPECT_EQ(memory[0].address, 0x3b000000);
    EXPECT_EQ(memory[0].numberOfPages, 0x04000000 >> 12);
}
