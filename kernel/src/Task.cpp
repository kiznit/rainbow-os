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

#include "Task.hpp"
#include "memory.hpp"
#include <atomic>

extern "C" void TaskSwitch(TaskContext** oldContext, TaskContext* newContext);

namespace
{
    std::atomic<int> s_nextTaskId;
}

void* Task::operator new(size_t size) noexcept
{
    assert(kTaskPageCount * mtl::kMemoryPageSize >= size);

    if (auto pages = AllocPages(kTaskPageCount))
        return pages.value();

    return nullptr;
}

void Task::operator delete(void* memory)
{
    FreePages(memory, kTaskPageCount);
}

std::shared_ptr<Task> Task::Create(EntryPoint* entryPoint, const void* args)
{
    // We can't use std::make_shared<> because that will use the global new operator and ignore our custom version.
    // TODO: simulate std::make_shared<> by using implementation details (RefCountWithObject) or other custom method.
    return std::shared_ptr<Task>(new Task(entryPoint, args));
}

Task::Task(EntryPoint* entryPoint, const void* args) : m_id(++s_nextTaskId)
{
    Initialize(entryPoint, args);

    assert(m_context != nullptr);
}

void Task::Bootstrap()
{
    assert(m_id == 1 && "Bootstrap() should only be used for the initial task");

    m_state = TaskState::Running;

    TaskContext* dummyContext;
    TaskSwitch(&dummyContext, m_context);

    __builtin_unreachable();
}

void Task::Entry(Task* task, EntryPoint entryPoint, const void* args)
{
    // MTL_LOG(Info) << "Task::Entry(): entryPoint " << (void*)entryPoint << ", args " << args;

    task->m_state = TaskState::Running;

    entryPoint(task, args);

    // TODO: die
    for (;;)
        ;
}
