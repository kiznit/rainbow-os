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
#include <list>
#include <metal/log.hpp>

namespace
{
    typedef std::list<Task*> ReadyQueue; // TODO: inefficient

    ReadyQueue g_readyQueue; // Tasks ready to run
} // namespace

namespace Scheduler
{

    void Initialize(Task* initialTask)
    {
        initialTask->Bootstrap();
    }

    void AddTask(Task* task)
    {
        g_readyQueue.emplace_back(task);
    }

    void Yield()
    {
        if (g_readyQueue.empty())
            return;

        const auto currentTask = Cpu::GetTask();
        const auto nextTask = g_readyQueue.front();

        g_readyQueue.pop_front();
        g_readyQueue.push_back(currentTask); // TODO: make sure this cannot fail (out of memory)

        currentTask->SwitchTo(nextTask);
    }

} // namespace Scheduler