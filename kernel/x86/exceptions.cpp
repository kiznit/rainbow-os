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

#include <kernel/interrupt.hpp>
#include <kernel/kernel.hpp>

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


static void dump_exception(const char* exception, const InterruptContext* context, void* address)
{
#if defined(__i386__)

    Log("\nEXCEPTION: %s, error %p, task %d, address %p\n", exception, context->error, cpu_get_data(task)->id, address);
    Log("    eax: %p    cs    : %p\n", context->eax, context->cs);
    Log("    ebx: %p    ds    : %p\n", context->ebx, context->ds);
    Log("    ecx: %p    es    : %p\n", context->ecx, context->es);
    Log("    edx: %p    fs    : %p\n", context->edx, context->fs);
    Log("    ebp: %p    gs    : %p\n", context->ebp, context->gs);
    Log("    esi: %p    ss    : %p\n", context->esi, context->ss);
    Log("    edi: %p    eflags: %p\n", context->edi, context->eflags);
    Log("    esp: %p    eip   : %p\n", context->esp, context->eip);

    const intptr_t* stack = (intptr_t*)context->esp;
    for (int i = 0; i != 10; ++i)
    {
        Log("    stack[%d]: %p\n", i, stack[i]);
    }

#elif defined(__x86_64__)

    Log("\nEXCEPTION: %s, error %p, task %d, address %p\n", exception, context->error, cpu_get_data(task)->id, address);
    Log("    rax: %p    r8    : %p\n", context->rax, context->r8);
    Log("    rbx: %p    r9    : %p\n", context->rbx, context->r9);
    Log("    rcx: %p    r10   : %p\n", context->rcx, context->r10);
    Log("    rdx: %p    r11   : %p\n", context->rdx, context->r11);
    Log("    rbp: %p    r12   : %p\n", context->rbp, context->r12);
    Log("    rsi: %p    r13   : %p\n", context->rsi, context->r13);
    Log("    rdi: %p    r14   : %p\n", context->rdi, context->r14);
    Log("    rsp: %p    r15   : %p\n", context->rsp, context->r15);
    Log("    rsp: %p    r15   : %p\n", context->rsp, context->r15);
    Log("    cs : %p    rflags: %p\n", context->cs, context->rflags);
    Log("    ss : %p    rip   : %p\n", context->ss, context->rip);

    const intptr_t* stack = (intptr_t*)context->rsp;
    for (int i = 0; i != 10; ++i)
    {
        Log("    stack[%d]: %p\n", i, stack[i]);
    }

#endif
}


#if defined(__i386__)
    #define UNHANDLED_EXCEPTION(vector, name) \
        extern "C" void exception_##name(InterruptContext* context) \
        { \
            dump_exception(#name, context, 0); \
            Fatal("Unhandled CPU exception: %x (%s)", vector, #name); \
        }
#elif defined(__x86_64__)
    #define UNHANDLED_EXCEPTION(vector, name) \
        extern "C" void exception_##name(InterruptContext* context) \
        { \
            dump_exception(#name, context, 0); \
            Fatal("Unhandled CPU exception: %x (%s)", vector, #name); \
        }
#endif


UNHANDLED_EXCEPTION( 0, divide_error)
UNHANDLED_EXCEPTION( 1, debug)
UNHANDLED_EXCEPTION( 2, nmi)
UNHANDLED_EXCEPTION( 3, breakpoint)
UNHANDLED_EXCEPTION( 4, overflow)
UNHANDLED_EXCEPTION( 5, bound_range_exceeded)
UNHANDLED_EXCEPTION( 6, invalid_opcode)
UNHANDLED_EXCEPTION( 8, double_fault)
UNHANDLED_EXCEPTION(10, invalid_tss)
UNHANDLED_EXCEPTION(11, stack_segment)
UNHANDLED_EXCEPTION(12, stack)
UNHANDLED_EXCEPTION(13, general)
UNHANDLED_EXCEPTION(16, fpu)
UNHANDLED_EXCEPTION(17, alignment)
UNHANDLED_EXCEPTION(18, machine_check)
UNHANDLED_EXCEPTION(19, simd)


// TODO: this is x86 specific and doesn't belong here...
extern "C" int exception_page_fault(InterruptContext* context, void* address)
{
    // Note: errata: Not-Present Page Faults May Set the RSVD Flag in the Error Code
    // Reference: https://www.intel.com/content/dam/www/public/us/en/documents/specification-updates/xeon-5400-spec-update.pdf
    // The right thing to do is ignore the "RSVD" flag if "P = 0".
    auto error = context->error;

    if (!(error & PAGEFAULT_PRESENT))
    {
        const auto task = cpu_get_data(task);

        // Is this a user stack access?
        if (address >= task->userStackTop && address < task->userStackBottom)
        {
            // We keep the first page as a guard page
            const auto page = (void*)align_down(address, MEMORY_PAGE_SIZE);
            if (page > task->userStackTop)
            {
                const auto frame = pmm_allocate_frames(1);
                task->pageTable.MapPages(frame, page, 1, PAGE_PRESENT | PAGE_USER | PAGE_WRITE | PAGE_NX);
                return 1;
            }
            else
            {
                // This is the guard page
                // TODO: raise a "stack overflow" signal / exception
            }
        }
    }

    dump_exception("#PF", context, address);
    Fatal("#PF: address %p, error %p\n", address, error);

    return 0;
}
