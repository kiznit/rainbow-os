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

#include "interrupt.hpp"
#include "Cpu.hpp"
#include <Task.hpp>
#include <metal/arch.hpp>
#include <metal/log.hpp>

// clang-format off
#define INTERRUPT_TABLE \
    INTERRUPT(0) INTERRUPT(1) INTERRUPT(2) INTERRUPT(3) INTERRUPT(4) INTERRUPT(5) INTERRUPT(6) INTERRUPT_NULL(7) \
    INTERRUPT(8) INTERRUPT_NULL(9) INTERRUPT(10) INTERRUPT(11) INTERRUPT(12) INTERRUPT(13) INTERRUPT(14) INTERRUPT_NULL(15) \
    INTERRUPT(16) INTERRUPT(17) INTERRUPT(18) INTERRUPT(19) INTERRUPT_NULL(20) INTERRUPT_NULL(21) INTERRUPT_NULL(22) INTERRUPT_NULL(23)\
    INTERRUPT_NULL(24) INTERRUPT_NULL(25) INTERRUPT_NULL(26) INTERRUPT_NULL(27) INTERRUPT_NULL(28) INTERRUPT_NULL(29) INTERRUPT_NULL(30) INTERRUPT_NULL(31)\
    INTERRUPT(32) INTERRUPT(33) INTERRUPT(34) INTERRUPT(35) INTERRUPT(36) INTERRUPT(37) INTERRUPT(38) INTERRUPT(39)\
    INTERRUPT(40) INTERRUPT(41) INTERRUPT(42) INTERRUPT(43) INTERRUPT(44) INTERRUPT(45) INTERRUPT(46) INTERRUPT(47)\
    INTERRUPT(48) INTERRUPT(49) INTERRUPT(50) INTERRUPT(51) INTERRUPT(52) INTERRUPT(53) INTERRUPT(54) INTERRUPT(55)\
    INTERRUPT(56) INTERRUPT(57) INTERRUPT(58) INTERRUPT(59) INTERRUPT(60) INTERRUPT(61) INTERRUPT(62) INTERRUPT(63)\
    INTERRUPT(64) INTERRUPT(65) INTERRUPT(66) INTERRUPT(67) INTERRUPT(68) INTERRUPT(69) INTERRUPT(70) INTERRUPT(71)\
    INTERRUPT(72) INTERRUPT(73) INTERRUPT(74) INTERRUPT(75) INTERRUPT(76) INTERRUPT(77) INTERRUPT(78) INTERRUPT(79)\
    INTERRUPT(80) INTERRUPT(81) INTERRUPT(82) INTERRUPT(83) INTERRUPT(84) INTERRUPT(85) INTERRUPT(86) INTERRUPT(87)\
    INTERRUPT(88) INTERRUPT(89) INTERRUPT(90) INTERRUPT(91) INTERRUPT(92) INTERRUPT(93) INTERRUPT(94) INTERRUPT(95)\
    INTERRUPT(96) INTERRUPT(97) INTERRUPT(98) INTERRUPT(99) INTERRUPT(100) INTERRUPT(101) INTERRUPT(102) INTERRUPT(103)\
    INTERRUPT(104) INTERRUPT(105) INTERRUPT(106) INTERRUPT(107) INTERRUPT(108) INTERRUPT(109) INTERRUPT(110) INTERRUPT(111)\
    INTERRUPT(112) INTERRUPT(113) INTERRUPT(114) INTERRUPT(115) INTERRUPT(116) INTERRUPT(117) INTERRUPT(118) INTERRUPT(119)\
    INTERRUPT(120) INTERRUPT(121) INTERRUPT(122) INTERRUPT(123) INTERRUPT(124) INTERRUPT(125) INTERRUPT(126) INTERRUPT(127)\
    INTERRUPT(128) INTERRUPT(129) INTERRUPT(130) INTERRUPT(131) INTERRUPT(132) INTERRUPT(133) INTERRUPT(134) INTERRUPT(135)\
    INTERRUPT(136) INTERRUPT(137) INTERRUPT(138) INTERRUPT(139) INTERRUPT(140) INTERRUPT(141) INTERRUPT(142) INTERRUPT(143)\
    INTERRUPT(144) INTERRUPT(145) INTERRUPT(146) INTERRUPT(147) INTERRUPT(148) INTERRUPT(149) INTERRUPT(150) INTERRUPT(151)\
    INTERRUPT(152) INTERRUPT(153) INTERRUPT(154) INTERRUPT(155) INTERRUPT(156) INTERRUPT(157) INTERRUPT(158) INTERRUPT(159)\
    INTERRUPT(160) INTERRUPT(161) INTERRUPT(162) INTERRUPT(163) INTERRUPT(164) INTERRUPT(165) INTERRUPT(166) INTERRUPT(167)\
    INTERRUPT(168) INTERRUPT(169) INTERRUPT(170) INTERRUPT(171) INTERRUPT(172) INTERRUPT(173) INTERRUPT(174) INTERRUPT(175)\
    INTERRUPT(176) INTERRUPT(177) INTERRUPT(178) INTERRUPT(179) INTERRUPT(180) INTERRUPT(181) INTERRUPT(182) INTERRUPT(183)\
    INTERRUPT(184) INTERRUPT(185) INTERRUPT(186) INTERRUPT(187) INTERRUPT(188) INTERRUPT(189) INTERRUPT(190) INTERRUPT(191)\
    INTERRUPT(192) INTERRUPT(193) INTERRUPT(194) INTERRUPT(195) INTERRUPT(196) INTERRUPT(197) INTERRUPT(198) INTERRUPT(199)\
    INTERRUPT(200) INTERRUPT(201) INTERRUPT(202) INTERRUPT(203) INTERRUPT(204) INTERRUPT(205) INTERRUPT(206) INTERRUPT(207)\
    INTERRUPT(208) INTERRUPT(209) INTERRUPT(210) INTERRUPT(211) INTERRUPT(212) INTERRUPT(213) INTERRUPT(214) INTERRUPT(215)\
    INTERRUPT(216) INTERRUPT(217) INTERRUPT(218) INTERRUPT(219) INTERRUPT(220) INTERRUPT(221) INTERRUPT(222) INTERRUPT(223)\
    INTERRUPT(224) INTERRUPT(225) INTERRUPT(226) INTERRUPT(227) INTERRUPT(228) INTERRUPT(229) INTERRUPT(230) INTERRUPT(231)\
    INTERRUPT(232) INTERRUPT(233) INTERRUPT(234) INTERRUPT(235) INTERRUPT(236) INTERRUPT(237) INTERRUPT(238) INTERRUPT(239)\
    INTERRUPT(240) INTERRUPT(241) INTERRUPT(242) INTERRUPT(243) INTERRUPT(244) INTERRUPT(245) INTERRUPT(246) INTERRUPT(247)\
    INTERRUPT(248) INTERRUPT(249) INTERRUPT(250) INTERRUPT(251) INTERRUPT(252) INTERRUPT(253) INTERRUPT(254) INTERRUPT(255)
// clang-format on

// Defined in interrupt.S
#define INTERRUPT(x) extern "C" void InterruptEntry##x();
#define INTERRUPT_NULL(x)
INTERRUPT_TABLE
#undef INTERRUPT
#undef INTERRUPT_NULL

#define INTERRUPT(x) (void*)InterruptEntry##x,
#define INTERRUPT_NULL(x) nullptr,
static void* g_interruptInitTable[256] = {INTERRUPT_TABLE};
#undef INTERRUPT_NULL
#undef INTERRUPT

// TODO: for security reasons, the IDT should be remapped read-only once intialization is completed.
//       If someone manages to execute kernel code with a user stack (hello syscall/swapgs), the IDT
//       can be overwritten with malicous entries.
//       This seems a good idea in general to protect kernel structures visible to user space mappings.
InterruptTable::InterruptTable()
{
    for (int i = 0; i != 256; ++i)
    {
        const auto entry = g_interruptInitTable[i];
        if (entry)
            SetInterruptGate(m_idt[i], entry);
        else
            SetNull(m_idt[i]);
    }
}

void InterruptTable::Load()
{
    const mtl::IdtPtr idtPtr{sizeof(m_idt) - 1, m_idt};
    mtl::x86_lidt(idtPtr);
}

void InterruptTable::SetInterruptGate(mtl::IdtDescriptor& descriptor, void* entry)
{
    const auto address = (uintptr_t)entry;
    descriptor.offset_low = address & 0xFFFF;
    descriptor.selector = static_cast<uint16_t>(Selector::KernelCode);
    descriptor.flags = 0x8E00; // Interrupt gate, DPL=0, present
    descriptor.offset_mid = (address >> 16) & 0xFFFF;
    descriptor.offset_high = (address >> 32) & 0xFFFFFFFF;
    descriptor.reserved = 0;
}

void InterruptTable::SetNull(mtl::IdtDescriptor& descriptor)
{
    memset(&descriptor, 0, sizeof(descriptor));
}

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
    MTL_LOG(Debug) << "CPU EXCEPTION: " << exception << ", error " << mtl::hex(context->error) << ", task "
                   << Task::GetCurrent()->GetId();

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

extern "C" void InterruptDispatch(InterruptContext* context)
{
    MTL_LOG(Fatal) << "Unhandled interrupt: IRQ " << context->interrupt;
    std::abort();
}
