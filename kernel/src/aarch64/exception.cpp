/*
    Copyright (c) 2023, Thierry Tremblay
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

#include "Cpu.hpp"
#include "interrupt.hpp"

static void LogException(const char* exception, const InterruptContext* context)
{
    const auto task = Cpu::GetCurrentTask();
    int taskId = task ? task->GetId() : -1;

    MTL_LOG(Debug) << "CPU EXCEPTION: " << exception << ", ESR_EL1 " << mtl::hex(mtl::Read_ESR_EL1()) << ", FAR_EL1 "
                   << mtl::hex(mtl::Read_FAR_EL1()) << ", ELR_EL1 " << mtl::hex(mtl::Read_ELR_EL1()) << ", task " << taskId;

    MTL_LOG(Debug) << "    x0 : " << mtl::hex(context->x0) << "    x8 : " << mtl::hex(context->x8)
                   << "    x16: " << mtl::hex(context->x16) << "    x24: " << mtl::hex(context->x24);
    MTL_LOG(Debug) << "    x1 : " << mtl::hex(context->x1) << "    x9 : " << mtl::hex(context->x9)
                   << "    x17: " << mtl::hex(context->x17) << "    x25: " << mtl::hex(context->x25);
    MTL_LOG(Debug) << "    x2 : " << mtl::hex(context->x2) << "    x10: " << mtl::hex(context->x10)
                   << "    x18: " << mtl::hex(context->x18) << "    x26: " << mtl::hex(context->x26);
    MTL_LOG(Debug) << "    x3 : " << mtl::hex(context->x3) << "    x11: " << mtl::hex(context->x11)
                   << "    x19: " << mtl::hex(context->x19) << "    x27: " << mtl::hex(context->x27);
    MTL_LOG(Debug) << "    x4 : " << mtl::hex(context->x4) << "    x12: " << mtl::hex(context->x12)
                   << "    x20: " << mtl::hex(context->x20) << "    x28: " << mtl::hex(context->x28);
    MTL_LOG(Debug) << "    x5 : " << mtl::hex(context->x5) << "    x13: " << mtl::hex(context->x13)
                   << "    x21: " << mtl::hex(context->x21) << "    fp : " << mtl::hex(context->fp);
    MTL_LOG(Debug) << "    x6 : " << mtl::hex(context->x6) << "    x14: " << mtl::hex(context->x14)
                   << "    x22: " << mtl::hex(context->x22) << "    lr : " << mtl::hex(context->lr);
    MTL_LOG(Debug) << "    x7 : " << mtl::hex(context->x7) << "    x15: " << mtl::hex(context->x15)
                   << "    x23: " << mtl::hex(context->x23) << "    sp : " << mtl::hex(context->sp);

    (void)context;
}

#define UNHANDLED_EXCEPTION(name)                                                                                                  \
    extern "C" void Exception_##name(InterruptContext* context)                                                                    \
    {                                                                                                                              \
        LogException(#name, context);                                                                                              \
        MTL_LOG(Fatal) << "Unhandled CPU exception: " << #name;                                                                    \
        std::abort();                                                                                                              \
    }

// Current EL with SP0
UNHANDLED_EXCEPTION(EL1t_SP0_Synchronous)
UNHANDLED_EXCEPTION(EL1t_SP0_IRQ)
UNHANDLED_EXCEPTION(EL1t_SP0_FIQ)
UNHANDLED_EXCEPTION(EL1t_SP0_SystemError)

// Current EL with SPx
UNHANDLED_EXCEPTION(EL1h_SPx_Synchronous)
// UNHANDLED_EXCEPTION(EL1h_SPx_IRQ)
UNHANDLED_EXCEPTION(EL1h_SPx_FIQ)
UNHANDLED_EXCEPTION(EL1h_SPx_SystemError)

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
