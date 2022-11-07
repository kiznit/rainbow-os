/*
    Copyright (c) 2022, Thierry Tremblay
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

#include "interrupt.hpp"
#include <metal/arch.hpp>
#include <metal/log.hpp>

void InterruptInit()
{
    extern void* ExceptionVectorEL1;
    mtl::Write_VBAR_EL1((uintptr_t)&ExceptionVectorEL1);
}

#define UNHANDLED_EXCEPTION(name)                                                                                                  \
    extern "C" void Exception_##name(InterruptContext* context)                                                                    \
    {                                                                                                                              \
        MTL_LOG(Fatal) << "Unhandled CPU exception: " << #name << ", lr " << mtl::hex(context->lr) << ", esr "                     \
                       << mtl::hex(mtl::Read_ESR_EL1()) << ", far " << mtl::hex(mtl::Read_FAR_EL1());                              \
        std::abort();                                                                                                              \
    }

// Current EL with SPx
UNHANDLED_EXCEPTION(EL1_SP0_Synchronous)
UNHANDLED_EXCEPTION(EL1_SP0_IRQ)
UNHANDLED_EXCEPTION(EL1_SP0_FIQ)
UNHANDLED_EXCEPTION(EL1_SP0_SystemError)

// Current EL with SPx
UNHANDLED_EXCEPTION(EL1_SPx_Synchronous)
UNHANDLED_EXCEPTION(EL1_SPx_IRQ)
UNHANDLED_EXCEPTION(EL1_SPx_FIQ)
UNHANDLED_EXCEPTION(EL1_SPx_SystemError)

// Lower EL using aarch64
UNHANDLED_EXCEPTION(EL0_64_Synchronous)
UNHANDLED_EXCEPTION(EL0_64_IRQ)
UNHANDLED_EXCEPTION(EL0_64_FIQ)
UNHANDLED_EXCEPTION(EL0_64_SystemError)

// Lower EL using aarch32
UNHANDLED_EXCEPTION(EL0_32_Synchronous)
UNHANDLED_EXCEPTION(EL0_32_IRQ)
UNHANDLED_EXCEPTION(EL0_32_FIQ)
UNHANDLED_EXCEPTION(EL0_32_SystemError)
