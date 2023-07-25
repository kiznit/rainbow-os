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

#include "Scheduler.hpp"
#include "Cpu.hpp"
#include <cassert>
#include <metal/log.hpp>

void Scheduler::Initialize(std::shared_ptr<Task> initialTask)
{
    initialTask->Bootstrap();
}

void Scheduler::AddTask(std::shared_ptr<Task> task)
{
    m_readyQueue.emplace_back(std::move(task));
}

void Scheduler::Yield()
{
    if (m_readyQueue.empty())
        return;

    // Note: we can't keep active shared pointers on the stack here as they might never get released.

    m_readyQueue.push_back(Task::GetCurrent()); // TODO: make sure this cannot fail (out of memory)
    auto previousTask = m_readyQueue.back().get();

    auto nextTask = m_readyQueue.front().get();
    Cpu::GetCurrent().SetTask(m_readyQueue.front());
    m_readyQueue.pop_front();

    previousTask->SwitchTo(nextTask);
}
