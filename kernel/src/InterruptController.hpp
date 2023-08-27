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

#pragma once

#include "ErrorCode.hpp"
#include <expected>

struct IInterruptHandler;
struct InterruptContext;

struct IInterruptController
{
    virtual ~IInterruptController() = default;

    // Initialize the interrupt controller
    virtual std::expected<void, ErrorCode> Initialize() = 0;

    // Register an interrupt handler
    virtual std::expected<void, ErrorCode> RegisterHandler(int interrupt, IInterruptHandler* handler) = 0;

    // Acknowledge an interrupt (End of interrupt / EOI)
    // TODO: do we need this now that controllers handle interrupts?
    virtual void Acknowledge(int interrupt) = 0;

    // Enable the specified interrupt
    virtual void Enable(int interrupt) = 0;

    // Disable the specified interrupt
    virtual void Disable(int interrupt) = 0;

    // Handle an interrupt
    virtual void HandleInterrupt(InterruptContext* context) = 0;
};

// TODO: this might be x86 specific
std::expected<void, ErrorCode> InterruptRegisterController(IInterruptController* interruptController);
