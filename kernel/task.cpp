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

#include "task.hpp"
#include <atomic>
#include <cassert>
#include <cstring>
#include <iterator>
#include <unordered_map>
#include <kernel/biglock.hpp>
#include <kernel/kernel.hpp>


// TODO: should not be visible outside
/*static*/ std::atomic_int s_nextTaskId;

static std::unordered_map<Task::Id, Task*> s_tasks;


void* Task::operator new(size_t size)
{
    assert(STACK_PAGE_COUNT * MEMORY_PAGE_SIZE >= size);
    return vmm_allocate_pages(STACK_PAGE_COUNT);
}


void Task::operator delete(void* p)
{
    vmm_free_pages(p, STACK_PAGE_COUNT);
}


Task* Task::Get(Id id)
{
    auto it = s_tasks.find(id);
    return it != s_tasks.end() ? it->second : nullptr;
}


Task::Task()
:   m_id(s_nextTaskId++),
    m_state(STATE_INIT)
{
    s_tasks[m_id] = this;
}


Task::Task(EntryPoint entryPoint, int flags, const void* args, size_t sizeArgs)
:   Task()
{
    m_pageTable = cpu_get_data(task)->m_pageTable;

    if (!(flags & CREATE_SHARE_PAGE_TABLE))
    {
        if (!m_pageTable.CloneKernelSpace())
        {
            // TODO: Have CloneKernelSpace() throw
            throw std::runtime_error("Could not clone kernel space");
        }
    }

    // If args is an object, we want to copy it somewhere inside the new thread's context.
    // The top of the stack works just fine (for now?).
    if (sizeArgs > 0)
    {
        memcpy((void*)(this + 1), args, sizeArgs);
        args = this + 1;
    }

    ArchInit(entryPoint, args);

    // Schedule the task
    m_state = STATE_READY;
    sched_add_task(this);
}


Task::~Task()
{
    s_tasks.erase(m_id);
}


void Task::Idle()
{
    for (;;)
    {
        // Verify that we have the lock
        assert(g_bigKernelLock.owner() == cpu_get_data(id));

        // TEMP: if there is any task to run, do not go idle
        // TODO: need better handling here, ideally the idle task doesn't get to run at all
        if (sched_pending_work())
        {
            sched_schedule();
        }

        // "else" is commented out for now, otherwise a CPU can get into an infinite sched_schedule() loop between two idle tasks.
        // The problem is that sched_pending_work() above sees an idle task and thinks there is work to do and switch to it.
        // That idle task in turn calls sched_pending_work() and see the previous idle task and switch to it. This creates a ping-pong
        // scheduling between the two idle tasks. What's worst, the kernel lock is never released and all the other CPUs are blocked.
        // The proper fix is a better scheduler and/or better handling of idle tasks. Task priorities could help as well.

        //else
        {
            g_bigKernelLock.unlock();
            interrupt_enable();

            // TODO: here we really want to halt, not pause... but we don't have a way to wakeup the halted CPU yet
            //x86_halt();
            x86_pause();

            interrupt_disable();
            g_bigKernelLock.lock();
        }
    }
}


void Task::Entry(Task* task, EntryPoint entryPoint, const void* args) noexcept
{
    assert(!interrupt_enabled());
    assert(g_bigKernelLock.owner() == cpu_get_data(id));

    //Log("Task::Entry(), id %d, entryPoint %p, args %p\n", task->id, entryPoint, args);

    try
    {
        entryPoint(task, args);
    }
    catch(...)
    {
        // It is possible to get here without having the lock
        // (an exception could be thrown outside the lock)
        // TODO: is this true?
        if (g_bigKernelLock.owner() != task->m_id)
        {
            g_bigKernelLock.lock();
        }

        Log("Unhandled exception in task %d\n", task->m_id);
    }

    assert(g_bigKernelLock.owner() == cpu_get_data(id));

    Log("Task %d exiting\n", task->m_id);

    //todo: kill current task (i.e. zombify it)
    //todo: remove task from scheduler
    //todo: yield() / schedule()
    //todo: free the kernel stack
    //todo: free the task

    for (;;)
    {
        sched_schedule();
    }
}
