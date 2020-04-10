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

#if defined(__i386__)

#include <metal/x86/cpuid.hpp>
#include "boot.hpp"

bool CheckArch()
{
    bool ok;

#if defined(KERNEL_X86_64)
    const bool hasLongMode = cpuid_has_longmode();

    if (!hasLongMode) Log("    Processor does not support long mode (64 bits)\n");

    ok = hasLongMode;
#else
    const bool hasPae = cpuid_has_pae();
    const bool hasNx = cpuid_has_nx();

    if (!hasPae) Log("    Processor does not support Physical Address Extension (PAE)\n");
    if (!hasNx) Log("    Processor does not support no-execute memory protection (NX/XD)\n");

    ok = hasPae && hasNx;
#endif

    return ok;
}

#else

bool CheckArch()
{
    // We like all x86_64 processors, so nothing to check
    return true;
}

#endif
