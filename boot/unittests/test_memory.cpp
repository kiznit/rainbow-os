/*
    Copyright (c) 2016, Thierry Tremblay
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
#include "memory.hpp"


static const physaddr_t PAGE_MIN = 0;
static const physaddr_t PAGE_MAX = (((physaddr_t)-1) >> MEMORY_PAGE_SHIFT) + 1;



TEST(MemoryMap, Basics)
{
    MemoryMap map;
    EXPECT_EQ(map.size(), 0);

    // Add an empty entry, expect nothing to change
    map.AddBytes(MemoryType_Available, 0x00100000, 0);
    EXPECT_EQ(map.size(), 0);

    // Add some free memory
    map.AddBytes(MemoryType_Available, 0x00100000, MEMORY_PAGE_SIZE * 16);
    EXPECT_EQ(map.size(), 1);
    EXPECT_EQ(map[0].type, MemoryType_Available);
    EXPECT_EQ(map[0].address(), 0x00100000);
    EXPECT_EQ(map[0].pageCount(), 16);

    // Add some reserved memory
    map.AddBytes(MemoryType_Reserved, 0x00200000, MEMORY_PAGE_SIZE * 10);
    EXPECT_EQ(map.size(), 2);
    EXPECT_EQ(map[1].type, MemoryType_Reserved);
    EXPECT_EQ(map[1].address(), 0x00200000);
    EXPECT_EQ(map[1].pageCount(), 10);
}



TEST(MemoryMap, PartialPages)
{
    MemoryMap map;

    // Available memory: less than a page
    map.AddBytes(MemoryType_Available, 0x00100000, MEMORY_PAGE_SIZE - 1);
    EXPECT_EQ(map.size(), 0);

    // Available memory: properly rounded to page boundaries
    map.clear();
    map.AddBytes(MemoryType_Available, 0x00100000 + MEMORY_PAGE_SIZE / 2, MEMORY_PAGE_SIZE);
    EXPECT_EQ(map.size(), 0);

    map.clear();
    map.AddBytes(MemoryType_Available, 0x00100000 + MEMORY_PAGE_SIZE / 2, MEMORY_PAGE_SIZE * 2);
    EXPECT_EQ(map.size(), 1);
    EXPECT_EQ(map[0].type, MemoryType_Available);
    EXPECT_EQ(map[0].address(), 0x00100000 + MEMORY_PAGE_SIZE);
    EXPECT_EQ(map[0].pageCount(), 1);


    // Used memory: less than a page
    map.clear();
    map.AddBytes(MemoryType_Reserved, 0x00100000, MEMORY_PAGE_SIZE - 1);
    EXPECT_EQ(map.size(), 1);
    EXPECT_EQ(map[0].address(), 0x00100000);
    EXPECT_EQ(map[0].pageCount(), 1);

    // Used memory: properly rounded to page boundaries
    map.clear();
    map.AddBytes(MemoryType_Reserved, 0x00100000 + MEMORY_PAGE_SIZE / 2, MEMORY_PAGE_SIZE);
    EXPECT_EQ(map.size(), 1);
    EXPECT_EQ(map[0].address(), 0x00100000);
    EXPECT_EQ(map[0].pageCount(), 2);

    map.clear();
    map.AddBytes(MemoryType_Reserved, 0x00100000 + MEMORY_PAGE_SIZE / 2, MEMORY_PAGE_SIZE * 2);
    EXPECT_EQ(map.size(), 1);
    EXPECT_EQ(map[0].address(), 0x00100000);
    EXPECT_EQ(map[0].pageCount(), 3);
}



TEST(MemoryMap, Limits_Available)
{
    MemoryMap map;

    // 0 bytes of available memory
    map.AddBytes(MemoryType_Available, 0x00100000, 0);
    EXPECT_EQ(map.size(), 0);

    // Max bytes of available memory, starting at 0
    map.clear();
    map.AddBytes(MemoryType_Available, 0, (physaddr_t)-1);
    EXPECT_EQ(map.size(), 1);
    EXPECT_EQ(map[0].address(), 0);
    EXPECT_EQ(map[0].pageCount(), PAGE_MAX - 1);

    // Max bytes of available memory, starting at MEMORY_PAGE_SIZE
    map.clear();
    map.AddBytes(MemoryType_Available, MEMORY_PAGE_SIZE, (physaddr_t)-1);
    EXPECT_EQ(map.size(), 1);
    EXPECT_EQ(map[0].address(), MEMORY_PAGE_SIZE);
    EXPECT_EQ(map[0].pageCount(), PAGE_MAX - 1);

    // Max bytes of available memory, starting in the middle of the first page
    map.clear();
    map.AddBytes(MemoryType_Available, MEMORY_PAGE_SIZE / 2, (physaddr_t)-1);
    EXPECT_EQ(map.size(), 1);
    EXPECT_EQ(map[0].address(), MEMORY_PAGE_SIZE);
    EXPECT_EQ(map[0].pageCount(), PAGE_MAX - 1);

    // Max bytes of available memory, starting near the end of the address space
    map.clear();
    map.AddBytes(MemoryType_Available, (PAGE_MAX-1) << MEMORY_PAGE_SHIFT, (physaddr_t)-1);
    EXPECT_EQ(map.size(), 1);
    EXPECT_EQ(map[0].address(), (PAGE_MAX-1) << MEMORY_PAGE_SHIFT);
    EXPECT_EQ(map[0].pageCount(), 1);

    // 0 pages of available memory
    map.clear();
    map.AddPages(MemoryType_Available, 0x00100000, 0);
    EXPECT_EQ(map.size(), 0);

    // Max pages of available memory, starting at 0
    map.clear();
    map.AddPages(MemoryType_Available, 0, (physaddr_t)-1);
    EXPECT_EQ(map.size(), 1);
    EXPECT_EQ(map[0].address(), 0);
    EXPECT_EQ(map[0].pageCount(), PAGE_MAX);

    // Max pages of available memory, starting at MEMORY_PAGE_SIZE
    map.clear();
    map.AddPages(MemoryType_Available, MEMORY_PAGE_SIZE, (physaddr_t)-1);
    EXPECT_EQ(map.size(), 1);
    EXPECT_EQ(map[0].address(), MEMORY_PAGE_SIZE);
    EXPECT_EQ(map[0].pageCount(), PAGE_MAX - 1);

    // Max pages of available memory, starting in the middle of the first page
    map.clear();
    map.AddPages(MemoryType_Available, MEMORY_PAGE_SIZE / 2, (physaddr_t)-1);
    EXPECT_EQ( map.size(), 1);
    EXPECT_EQ(map[0].address(), MEMORY_PAGE_SIZE);
    EXPECT_EQ(map[0].pageCount(), PAGE_MAX - 1);

    // Max pages of available memory, starting near the end of the address space
    map.clear();
    map.AddPages(MemoryType_Available, (PAGE_MAX-1) << MEMORY_PAGE_SHIFT, (physaddr_t)-1);
    EXPECT_EQ(map.size(), 1);
    EXPECT_EQ(map[0].address(), (PAGE_MAX-1) << MEMORY_PAGE_SHIFT);
    EXPECT_EQ(map[0].pageCount(), 1);
}



TEST(MemoryMap, Limits_Reserved)
{
    MemoryMap map;

    // 0 bytes of reserved memory
    map.AddBytes(MemoryType_Reserved, 0x00100000, 0);
    EXPECT_EQ(map.size(), 0);

    // Max bytes of reserved memory, starting at 0
    map.clear();
    map.AddBytes(MemoryType_Reserved, 0, (physaddr_t)-1);
    EXPECT_EQ(map.size(), 1);
    EXPECT_EQ(map[0].address(), 0);
    EXPECT_EQ(map[0].pageCount(), PAGE_MAX);

    // Max bytes of reserved memory, starting at MEMORY_PAGE_SIZE
    map.clear();
    map.AddBytes(MemoryType_Reserved, MEMORY_PAGE_SIZE, (physaddr_t)-1);
    EXPECT_EQ(map.size(), 1);
    EXPECT_EQ(map[0].address(), MEMORY_PAGE_SIZE);
    EXPECT_EQ(map[0].pageCount(), PAGE_MAX - 1);

    // Max bytes of reserved memory, starting in the middle of the first page
    map.clear();
    map.AddBytes(MemoryType_Reserved, MEMORY_PAGE_SIZE / 2, (physaddr_t)-1);
    EXPECT_EQ(map.size(), 1);
    EXPECT_EQ(map[0].address(), 0);
    EXPECT_EQ(map[0].pageCount(), PAGE_MAX);

    // Max bytes of available memory, starting near the end of the address space
    map.clear();
    map.AddBytes(MemoryType_Reserved, (PAGE_MAX-1) << MEMORY_PAGE_SHIFT, (physaddr_t)-1);
    EXPECT_EQ(map.size(), 1);
    EXPECT_EQ(map[0].address(), (PAGE_MAX-1) << MEMORY_PAGE_SHIFT);
    EXPECT_EQ(map[0].pageCount(), 1);

    // 0 pages of reserved memory
    map.clear();
    map.AddPages(MemoryType_Reserved, 0x00100000, 0);
    EXPECT_EQ(map.size(), 0);

    // Max pages of reserved memory, starting at 0
    map.clear();
    map.AddPages(MemoryType_Reserved, 0, (physaddr_t)-1);
    EXPECT_EQ(map.size(), 1);
    EXPECT_EQ(map[0].address(), 0);
    EXPECT_EQ(map[0].pageCount(), PAGE_MAX);

    // Max pages of reserved memory, starting at MEMORY_PAGE_SIZE
    map.clear();
    map.AddPages(MemoryType_Reserved, MEMORY_PAGE_SIZE, (physaddr_t)-1);
    EXPECT_EQ(map.size(), 1);
    EXPECT_EQ(map[0].address(), MEMORY_PAGE_SIZE);
    EXPECT_EQ(map[0].pageCount(), PAGE_MAX - 1);

    // Max pages of reserved memory, starting in the middle of the first page
    map.clear();
    map.AddPages(MemoryType_Reserved, MEMORY_PAGE_SIZE / 2, (physaddr_t)-1);
    EXPECT_EQ( map.size(), 1);
    EXPECT_EQ(map[0].address(), 0);
    EXPECT_EQ(map[0].pageCount(), PAGE_MAX);

    // Max pages of available memory, starting near the end of the address space
    map.clear();
    map.AddPages(MemoryType_Reserved, (PAGE_MAX-1) << MEMORY_PAGE_SHIFT, (physaddr_t)-1);
    EXPECT_EQ(map.size(), 1);
    EXPECT_EQ(map[0].address(), (PAGE_MAX-1) << MEMORY_PAGE_SHIFT);
    EXPECT_EQ(map[0].pageCount(), 1);
}



TEST(MemoryMap, Allocations)
{
    MemoryMap map;

    physaddr_t memory;

    // Try to allocate when there is no memory
    memory = map.AllocateBytes(MemoryType_Bootloader, 100);
    EXPECT_EQ(memory, MEMORY_ALLOC_FAILED);
    memory = map.AllocatePages(MemoryType_Bootloader, 10);
    EXPECT_EQ(memory, MEMORY_ALLOC_FAILED);

    // Get some memory
    map.AddPages(MemoryType_Available, 5 * MEMORY_PAGE_SIZE, 95);

    // Allocating 0 bytes / pages should fail
    memory = map.AllocateBytes(MemoryType_Bootloader, 0);
    EXPECT_EQ(memory, MEMORY_ALLOC_FAILED);
    memory = map.AllocatePages(MemoryType_Bootloader, 0);
    EXPECT_EQ(memory, MEMORY_ALLOC_FAILED);

    // Allocating memory should come from highest available memory
    memory = map.AllocatePages(MemoryType_Bootloader, 10);
    EXPECT_EQ(memory, 90 * MEMORY_PAGE_SIZE);
    memory = map.AllocatePages(MemoryType_Bootloader, 5);
    EXPECT_EQ(memory, 85 * MEMORY_PAGE_SIZE);

    // Allocating memory should come from highest available memory
    map.AddPages(MemoryType_Available, 200 * MEMORY_PAGE_SIZE, 10);

    memory = map.AllocatePages(MemoryType_Kernel, 5);
    EXPECT_EQ(memory, 205 * MEMORY_PAGE_SIZE);
    memory = map.AllocatePages(MemoryType_Kernel, 10);
    EXPECT_EQ(memory, 75 * MEMORY_PAGE_SIZE);
    memory = map.AllocatePages(MemoryType_Kernel, 5);
    EXPECT_EQ(memory, 200 * MEMORY_PAGE_SIZE);

    map.Sanitize();

    // Verify final state of memory map
    EXPECT_EQ(map.size(), 4);

    EXPECT_EQ(map[0].type, MemoryType_Available);
    EXPECT_EQ(map[0].address(), 5 * MEMORY_PAGE_SIZE);
    EXPECT_EQ(map[0].pageCount(), 70);

    EXPECT_EQ(map[1].type, MemoryType_Kernel);
    EXPECT_EQ(map[1].address(), 75 * MEMORY_PAGE_SIZE);
    EXPECT_EQ(map[1].pageCount(), 10);

    EXPECT_EQ(map[2].type, MemoryType_Bootloader);
    EXPECT_EQ(map[2].address(), 85 * MEMORY_PAGE_SIZE);
    EXPECT_EQ(map[2].pageCount(), 15);

    EXPECT_EQ(map[3].type, MemoryType_Kernel);
    EXPECT_EQ(map[3].address(), 200 * MEMORY_PAGE_SIZE);
    EXPECT_EQ(map[3].pageCount(), 10);
}



TEST(MemoryMap, Allocation_MaxAddressDefaultsTo4GB)
{
    MemoryMap map;

    map.AddBytes(MemoryType_Available, 0, 0x9F000);
    map.AddBytes(MemoryType_Available, 0x100000, 0xbfefb000);
    map.AddBytes(MemoryType_Available, 0x100000000ull, 0x140000000ull);

    physaddr_t memory;

    // Make sure memory is all under 4GB so that it can be accessed in 32 bits mode
    memory = map.AllocateBytes(MemoryType_Bootloader, 300000);
    EXPECT_LT(memory, 0x100000000ull);
    EXPECT_LT(memory + 300000, 0x100000000ull);

    memory = map.AllocatePages(MemoryType_Kernel, 72);
    EXPECT_LT(memory, 0x100000000ull);
    EXPECT_LT(memory + 72 * MEMORY_PAGE_SIZE, 0x100000000ull);
}
