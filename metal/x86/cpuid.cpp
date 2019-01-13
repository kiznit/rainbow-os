/*
    Copyright (c) 2018, Thierry Tremblay
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

#include "cpuid.hpp"


static bool s_cpuDetected = false;
static bool s_isIntel = false;
static bool s_isAMD = false;
static unsigned int s_processorId = 0;



static void x86_detect_cpu()
{
    unsigned int eax, ebx, ecx, edx;

    if (__get_cpuid(0, &eax, &ebx, &ecx, &edx))
    {
        if (ebx == signature_AMD_ebx &&
            ecx == signature_AMD_ecx &&
            edx == signature_AMD_edx)
        {
            s_isAMD = true;
        }

        if (ebx == signature_INTEL_ebx &&
            ecx == signature_INTEL_ecx &&
            edx == signature_INTEL_edx)
        {
            s_isIntel = true;
        }
    }

    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx))
    {
        s_processorId = eax;
    }
}




unsigned int x86_cpuid_max(unsigned int ext, unsigned int* signature)
{
    return __get_cpuid_max(ext, signature);
}



int x86_cpuid(unsigned int leaf, unsigned int* eax, unsigned int* ebx, unsigned int* ecx, unsigned int* edx)
{
    if (!s_cpuDetected)
    {
        x86_detect_cpu();
        s_cpuDetected = true;
    }

    if (!__get_cpuid(leaf, eax, ebx, ecx, edx))
    {
        return 0;
    }

#if defined(__i386__)
    if (s_isIntel)
    {
        // Pentium M Banias doesn't have the PAE bit set, but it does support PAE
        if (s_processorId == INTEL_PENTIUM_M_BANIAS_SIGNATURE)
        {
            if (leaf == 1)
            {
                *edx |= bit_PAE;
            }
        }
    }
#endif

    return 1;
}


int x86_cpuid_count(unsigned int leaf, unsigned subleaf, unsigned int* eax, unsigned int* ebx, unsigned int* ecx, unsigned int* edx)
{
    return __get_cpuid_count(leaf, subleaf, eax, ebx, ecx, edx);
}
