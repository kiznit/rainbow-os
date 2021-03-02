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


// fixed takes precedence over variable when enabled
// variable: if (address & mask ) == (base & mask) --> MTRR applied
// overlapping ranges: if same -> ok, if UC -> wins, WT > WB


Mtrr::Mtrr()
{
    const auto caps = x86_read_msr(Msr::IA32_MTRRCAP);
    m_variableCount = caps & 0x7;
    m_fixedSupported = caps & (1 << 8);
    m_wcSupported = caps & (1 << 10);
    m_smrrSupported = caps & (1 << 11);

    const auto defType = x86_read_msr(Msr::IA32_MTRR_DEF_TYPE);
    m_defMemType = (MemoryType)(defType & 7);
    m_fixedEnabled = defType & (1 << 10);
    m_enabled = defType & (1 << 11);

}


void Mtrr::Log() const
{
    using ::Log;

    Log("MTRR support:\n");
    Log("   enabled         : %d\n", m_enabled);
    Log("   fixed range     : %d\n", m_fixedSupported);
    Log("   fixed enabled   : %d\n", m_fixedEnabled);
    Log("   variable count  : %d\n", m_variableCount);
    Log("   write combining : %d\n", m_wcSupported);
    Log("   smrr            : %d\n", m_smrrSupported);
    Log("   default mem type: %d (%s)\n", m_defMemType, s_memTypes[(int)m_defMemType]);

    if (m_fixedSupported)
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

    if (m_variableCount)
    {
        Log("MTRR variable ranges:\n");

        for (int i = 0; i != m_variableCount; ++i)
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
