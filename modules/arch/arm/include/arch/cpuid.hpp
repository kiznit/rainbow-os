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

/*
    The Main ID Register (MIDR) uniquely identifies a processor model.

    Here is a breakdown ofd the MIDR register:

        0xFF000000 - Implementor
        0x00F00000 - Variant (Major revision number)
        0x000F0000 - Architecture format description
        0x0000FFF0 - Part number
        0x0000000F - Revision number
*/



// Part number:
//  ARM1176     : 0x410fb767    0x410fb767 on Raspberry Pi 1
//  Cortex-A7   : 0x410fc070    0x410fc075 on Raspberry Pi 2
//  Cortex-A53  : 0x410fd034    0x410fd034 on Raspberry Pi 3

// Implementators
#define ARM_CPU_IMPL_ARM        0x41
#define ARM_CPU_IMPL_INTEL      0x69

// Models are a combinaison of implementor and part number.
#define ARM_CPU_MODEL_ARM1176   0x4100b760
#define ARM_CPU_MODEL_CORTEXA7  0x4100c070
#define ARM_CPU_MODEL_CORTEXA53 0x4100d030
#define ARM_CPU_MODEL_MASK      0xff00fff0


inline unsigned arm_cpuid_id()
{
    // Retrieve the processor's Main ID Register (MIDR)
    unsigned value;
    asm("mrc p15,0,%0,c0,c0,0" : "=r"(value) : : "cc");
    return value;
}

inline unsigned arm_cpuid_model()
{
    return arm_cpuid_id() & ARM_CPU_MODEL_MASK;
}
