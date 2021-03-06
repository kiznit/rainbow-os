/*
    Copyright (c) 2020, Thierry Tremblay
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

#ifndef _RAINBOW_METAL_X86_CPUID_HPP
#define _RAINBOW_METAL_X86_CPUID_HPP

#include <cpuid.h>


// Some missing definitions from cpuid.h for %edx features
#define bit_PAE (1 << 6)
#define bit_PAT (1 << 16)
#define bit_LONG_MODE (1 << 29)

// 0x80000001, edx
#define bit_NX (1 << 20)
#define bit_Page1GB (1 << 26)

// Vendor processor ids
#define INTEL_PENTIUM_M_BANIAS_SIGNATURE 0x695


/*
    cpuid - this is different from calling __get_cpuid() directly because
            it "applies fixes" to know processors
*/

unsigned int x86_cpuid_max(unsigned int ext=0, unsigned int* signature=nullptr);
int x86_cpuid(unsigned int leaf, unsigned int* eax, unsigned int* ebx, unsigned int* ecx, unsigned int* edx);
int x86_cpuid_count(unsigned int leaf, unsigned subleaf, unsigned int* eax, unsigned int* ebx, unsigned int* ecx, unsigned int* edx);


bool cpuid_has_1gbpage();
bool cpuid_has_longmode();
bool cpuid_has_nx();
bool cpuid_has_pae();
bool cpuid_has_pat();

#endif
