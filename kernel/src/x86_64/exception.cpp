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
#include "Task.hpp"
#include "interrupt.hpp"

/*
    x86 CPU exceptions
    0   #DE - Divide Error                  16  #MF - Floating-Point Error
    1   #DB - Debug                         17  #AC - Alignment Check
    2         NMI                           18  #MC - Machine Check
    3   #BP - Breakpoint                    19  #XM/#XF - SIMD Floating-Point Error
    4   #OF - Overflow                      20  #VE - Virtualization Exception
    5   #BR - BOUND Range Exceeded          21  - Reserved -
    6   #UD - Invalid Opcode                22  - Reserved -
    7   #NM - Device Not Available          23  - Reserved -
    8   #DF - Double Fault                  24  - Reserved -
    9   - Reserved -                        25  - Reserved -
    10  #TS - Invalid TSS                   26  - Reserved -
    11  #NP - Segment Not Present           27  - Reserved -
    12  #SS - Stack Fault                   28  #HV - Hypervisor Injection Exception (AMD only?)
    13  #GP - General Protection            29  #VC - VMM Communication Exception (AMD only?)
    14  #PF - Page Fault                    30  #SX - Security Exception (AMD only?)
    15  - Reserved -                        31   - Reserved -
    The following CPU exceptions will push an error code: 8, 10-14, 17, 30.
*/

static void LogException(const char* exception, const InterruptContext* context)
{
    const auto task = CpuGetTask();
    int taskId = task ? task->GetId() : -1;

    MTL_LOG(Debug) << "CPU EXCEPTION: " << exception << ", error " << mtl::hex(context->error) << ", task " << taskId;

    MTL_LOG(Debug) << "    rax: " << mtl::hex(context->rax) << "    rbp: " << mtl::hex(context->rbp)
                   << "    r8 : " << mtl::hex(context->r8) << "    r12   : " << mtl::hex(context->r12);
    MTL_LOG(Debug) << "    rbx: " << mtl::hex(context->rbx) << "    rsi: " << mtl::hex(context->rsi)
                   << "    r9 : " << mtl::hex(context->r9) << "    r13   : " << mtl::hex(context->r13);
    MTL_LOG(Debug) << "    rcx: " << mtl::hex(context->rcx) << "    rdi: " << mtl::hex(context->rdi)
                   << "    r10: " << mtl::hex(context->r10) << "    r14   : " << mtl::hex(context->r14);
    MTL_LOG(Debug) << "    rdx: " << mtl::hex(context->rdx) << "    rsp: " << mtl::hex(context->rsp)
                   << "    r11: " << mtl::hex(context->r11) << "    r15   : " << mtl::hex(context->r15);
    MTL_LOG(Debug) << "    cs : " << mtl::hex(context->cs) << "    rip: " << mtl::hex(context->rip)
                   << "    ss : " << mtl::hex(context->ss) << "    rflags: " << mtl::hex(context->rflags);

    const auto stack = (uint64_t*)context->rsp;
    for (int i = 0; i != 10; ++i)
    {
        MTL_LOG(Debug) << "    stack[" << i << "]: " << mtl::hex(stack[i]);
    }
}

#define UNHANDLED_EXCEPTION(vector, name)                                                                                          \
    extern "C" void Exception##name(InterruptContext* context)                                                                     \
    {                                                                                                                              \
        LogException(#name, context);                                                                                              \
        MTL_LOG(Fatal) << "Unhandled CPU exception: " << mtl::hex<uint8_t>(vector) << " (" #name ")";                              \
        std::abort();                                                                                                              \
    }

UNHANDLED_EXCEPTION(0, DivideError)
UNHANDLED_EXCEPTION(1, Debug)
UNHANDLED_EXCEPTION(2, Nmi)
UNHANDLED_EXCEPTION(3, Breakpoint)
UNHANDLED_EXCEPTION(4, Overflow)
UNHANDLED_EXCEPTION(5, BoundRangeExceeded)
UNHANDLED_EXCEPTION(6, InvalidOpcode)
UNHANDLED_EXCEPTION(8, DoubleFault)
UNHANDLED_EXCEPTION(10, InvalidTss)
UNHANDLED_EXCEPTION(11, StackSegment)
UNHANDLED_EXCEPTION(12, Stack)
UNHANDLED_EXCEPTION(13, General)
UNHANDLED_EXCEPTION(16, Fpu)
UNHANDLED_EXCEPTION(17, Alignment)
UNHANDLED_EXCEPTION(18, MachineCheck)
UNHANDLED_EXCEPTION(19, Simd)

extern "C" void ExceptionPageFault(InterruptContext* context)
{
    const auto address = (void*)mtl::Read_CR2();
    LogException("PageFault", context);
    MTL_LOG(Fatal) << "Unhandled CPU exception: 0e (PageFault), address " << address;
    std::abort();
}
