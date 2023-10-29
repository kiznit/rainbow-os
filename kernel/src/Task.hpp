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

#include <cstddef>
#include <metal/arch.hpp>

class CpuContext;

enum class TaskState
{
    Init,    // Task is initializing
    Running, // Task is running
    Ready,   // Task is ready to run
};

class Task
{
public:
    using EntryPoint = void(Task* task, const void* args);
    using Id = int;

    // Allocate / free a task
    void* operator new(size_t size) noexcept;
    void operator delete(void* p);

    // Private constructor because we use a custom allocator
    Task(EntryPoint* entryPoint, const void* args);

    // No copy / assignment
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    // Bootstrap task 0
    [[noreturn]] void Bootstrap();

    int GetId() const { return m_id; }
    TaskState GetState() const { return m_state; }

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

    const Id m_id;
    TaskState m_state{TaskState::Init};
    CpuContext* m_context{}; // Saved CPU context (on the task's stack)
};
