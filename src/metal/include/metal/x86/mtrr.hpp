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

#ifndef _RAINBOW_METAL_X86_MTRR_HPP
#define _RAINBOW_METAL_X86_MTRR_HPP

#include <metal/cpu.hpp>


class Mtrr
{
public:

    //using Callback = void (*)(MtrrMemoryType memoryType, physaddr_t base, physaddr_t mask);

    enum class MemoryType
    {
        UC = 0, // Uncacheable
        WC = 1, // Write Combining
        WT = 4, // Write-through
        WP = 5, // Write-protected
        WB = 6, // Writeback
    };

    Mtrr();

    //void EnumerateRanges(Callback callback);

    void Log() const;

private:

    // IA32_MTRRCAP
    int         m_variableCount;    // Count of MTRR variable ranges
    bool        m_fixedSupported;   // Are fixed ranges supported?
    bool        m_wcSupported;      // Is write-combining supported?
    bool        m_smrrSupported;    // Is SMRR supported?

    // IA32_MTRR_DEF_TYPE
    MemoryType  m_defMemType;       // Default memory type when MTRRs enabled
    bool        m_fixedEnabled;     // Are fixed ranges enabled?
    bool        m_enabled;          // Are MTRRs enabled?

};


#endif
