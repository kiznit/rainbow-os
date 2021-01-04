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


#include "reent.hpp"
#include <cassert>
#include <cstring>
#include <iterator>
#include <reent.h>
#include <metal/cpu.hpp>


struct ReentContext
{
    _reent      newlib;
    FpuState    fpu;
};


// TODO: what's the right upper bound here?
// TODO: if we want to support reentrancy at some point, we will need this per-cpu
// TODO: move to percpu data and/or TLS?
static ReentContext  s_contexts[8];
static ReentContext* s_current;


void reent_init()
{
    s_current = &s_contexts[0];

    // Initialize newlib context
    _impure_ptr = &s_current->newlib;
    _REENT_INIT_PTR_ZEROED(_impure_ptr);
}


void reent_push()
{
    // Save the FPU state
    fpu_save(&s_current->fpu);

    // TODO: do we need to reinitialize the FPU in any way? Perhaps control words?

    // Allocate context
    assert((uintptr_t)(s_current - s_contexts) < std::size(s_contexts));
    ++s_current;

    // Initialize the newlib context (we don't know if it is zero-ed, so we need to first clear the object)
    _impure_ptr = &s_current->newlib;
    memset(_impure_ptr, 0, sizeof(*_impure_ptr));
    _REENT_INIT_PTR_ZEROED(_impure_ptr);
}


void reent_pop()
{
    // Free current context
    --s_current;
    assert(s_current >= s_contexts);

    // Restore newlib context
    _impure_ptr = &s_current->newlib;

    // Restore fpu state
    fpu_restore(&s_current->fpu);
}
