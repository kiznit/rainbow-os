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

#pragma once

#include <cstddef>
#include <metal/arch.hpp>

#if defined(__x86_64__)
#include "x86_64/task.hpp"
#elif defined(__aarch64__)
#include "aarch64/task.hpp"
#endif

class Task
{
public:
    using EntryPoint = void(Task* task, const void* args);

    // Allocate / free a task
    void* operator new(size_t size) noexcept;
    void operator delete(void* p);

    // Bootstrap task 0
    Task(EntryPoint* entryPoint, const void* args);

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    // Bootstrap task 0
    [[noreturn]] void Bootstrap();

    // Switch task
    void SwitchTo(Task* newTask);

private:
    static constexpr auto kTaskPageCount = 2;

    // Platform specific initialization
    void Initialize(EntryPoint entryPoint, const void* args);

    // Entry point for new tasks
    static void Entry(Task* task, EntryPoint entryPoint, const void* args) __attribute__((noreturn));

    // Get kernel stack top / bottom
    constexpr void* GetStackTop() const { return (void*)(this + 1); }
    constexpr void* GetStack() const { return (char*)this + kTaskPageCount * mtl::kMemoryPageSize; }

    TaskContext* m_context; // Saved CPU context (on the task's stack)
};