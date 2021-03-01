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

#include <metal/x86/mtrr.hpp>
#include <cassert>
#include <metal/helpers.hpp>
#include <metal/log.hpp>

struct FixedRangeMtrr
{
    Msr msr;
    uint32_t address;
    uint32_t size;
};

static const char* s_memTypes[8] =
{
    "UC",
    "WC",
    "**",
    "**",
    "WT",
    "WP",
    "WB",
    "**"
};

static const FixedRangeMtrr s_fixedRanges[11] =
{
    { Msr::IA32_MTRR_FIX64K_00000,  0x00000, 0x80000 },
    { Msr::IA32_MTRR_FIX16K_80000,  0x80000, 0x20000 },
    { Msr::IA32_MTRR_FIX16K_A0000,  0xA0000, 0x20000 },
    { Msr::IA32_MTRR_FIX4K_C0000,   0xC0000, 0x08000 },
    { Msr::IA32_MTRR_FIX4K_C8000,   0xC8000, 0x08000 },
    { Msr::IA32_MTRR_FIX4K_D0000,   0xD0000, 0x08000 },
    { Msr::IA32_MTRR_FIX4K_D8000,   0xD8000, 0x08000 },
    { Msr::IA32_MTRR_FIX4K_E0000,   0xE0000, 0x08000 },
    { Msr::IA32_MTRR_FIX4K_E8000,   0xE8000, 0x08000 },
    { Msr::IA32_MTRR_FIX4K_F0000,   0xF0000, 0x08000 },
    { Msr::IA32_MTRR_FIX4K_F8000,   0xF8000, 0x08000 },
};


void mtrr_log()
{
    const auto cap = x86_read_msr(Msr::IA32_MTRRCAP);

    const int vcnt = cap & 0x7;
    const bool fixed = cap & (1 << 8);
    const bool wc = cap & (1 << 10);
    const bool smrr = cap & (1 << 11);
    const int defaultMemType = x86_read_msr(Msr::IA32_MTRR_DEF_TYPE) & 7;

    Log("MTRR support:\n");
    Log("   vcnt            : %d\n", vcnt);
    Log("   fixed range     : %d\n", fixed);
    Log("   write combining : %d\n", wc);
    Log("   smrr            : %d\n", smrr);
    Log("   default mem type: %d (%s)\n", defaultMemType, s_memTypes[defaultMemType]);

    if (fixed)
    {
        Log("MTRR fixed ranges:\n");

        int regionStart;
        int regionSize;
        int regionMemType;

        for (const auto& range: s_fixedRanges)
        {
            uint64_t msr = x86_read_msr(range.msr);
            auto address = range.address;
            auto size = range.size / 8;

            for (int i = 0; i != 8; ++i)
            {
                const int memType = msr & 0x7;

                if (address == 0)
                {
                    // First fixed range, initialize region
                    regionStart = address;
                    regionSize = size;
                    regionMemType = memType;
                }
                else if (memType == regionMemType)
                {
                    // Same memory type as previous fixed range, update region
                    regionSize += size;
                }
                else
                {
                    // Different memory type
                    Log("    %08x - %08x: %d (%s)\n", regionStart, regionStart + regionSize, regionMemType, s_memTypes[regionMemType]);

                    regionStart = address;
                    regionSize = size;
                    regionMemType = memType;
                }

                msr >>= 8;
                address += size;
            }
        }

        // Print final region
        Log("    %08x - %08x: %d (%s)\n", regionStart, regionStart + regionSize, regionMemType, s_memTypes[regionMemType]);
    }

    if (vcnt)
    {
        Log("MTRR variable ranges:\n");

        for (int i = 0; i != vcnt; ++i)
        {
            auto base = x86_read_msr((Msr)((int)Msr::IA32_MTRR_PHYSBASE0 + 2 * i));
            const int memType = base & 0x7;
            base = base & ~0xFFF;

            auto mask = x86_read_msr((Msr)((int)Msr::IA32_MTRR_PHYSMASK0 + 2 * i));
            const int valid = mask & (1 << 11);
            mask = mask & ~0xFFF;

            if (valid)
            {
                Log("    %016jx - %016jx: %d (%s)\n", base, mask, memType, s_memTypes[memType]);
            }
        }
    }
}
