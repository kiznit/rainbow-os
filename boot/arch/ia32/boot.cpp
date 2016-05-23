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

#include "boot.hpp"
#include <cpuid.h>


#ifndef bit_PAE
#define bit_PAE (1 << 6)
#endif
#ifndef bit_LONG_MODE
#define bit_LONG_MODE (1 << 29)
#endif

#define INTEL_PENTIUM_M_BANIAS_SIGNATURE 0x695



bool VerifyCPU_ia32()
{
    bool isIntel = false;

    // Verify CPUID support
    if (__get_cpuid_max(0, NULL) < 1)
        return false;

    // Retrieve the vendor ID
    unsigned int eax, ebx, ecx, edx;
    if (!__get_cpuid(0, &eax, &ebx, &ecx, &edx))
        return false;

    if (ebx == signature_INTEL_ebx &&
        ecx == signature_INTEL_ecx &&
        edx == signature_INTEL_edx)
    {
        isIntel = true;
    }

    // Retrieve processor info and features
    if (!__get_cpuid(1, &eax, &ebx, &ecx, &edx))
        return false;

    // Intel fixes
    if (isIntel)
    {
        // Pentium M Banias doesn't have the PAE bit set, but it does support PAE
        if (eax == INTEL_PENTIUM_M_BANIAS_SIGNATURE)
        {
            edx |= bit_PAE;
        }
    }

    // We want SS2 and PAE
    if (!(edx & bit_FXSAVE) || !(edx & bit_SSE2) || !(edx & bit_PAE))
        return false;

    return true;
}



bool VerifyCPU_x86_64()
{
    // Verify CPUID support
    if (__get_cpuid_max(0x80000000, NULL) < 0x80000001)
        return false;

    // Retrieve extended features
    unsigned int eax, ebx, ecx, edx;
    if (!__get_cpuid(0x80000001, &eax, &ebx, &ecx, &edx))
        return false;

    // We want long mode
    if (!(edx & bit_LONG_MODE))
        return false;

    return true;
}
