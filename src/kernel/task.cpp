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
#include <metal/interrupt.hpp>
#include <metal/log.hpp>

#include "biglock.hpp"
#include "kernel.hpp"
#include "scheduler.hpp"
#include "vmm.hpp"

extern Scheduler g_scheduler;


// TODO: should not be visible outside
/*static*/ std::atomic_int s_nextTaskId;

// TODO: need a hash table or similar
static Task* s_tasks[1000];


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
    assert(id >= 0 && id < std::ssize(s_tasks));
    return s_tasks[id];
}


Task::Task(const std::shared_ptr<PageTable>& pageTable)
:   m_id(s_nextTaskId++),
    m_state(TaskState::Init),
    m_priority(TaskPriority::Normal),
    m_pageTable(pageTable)
{
    assert(!!m_pageTable);

    assert(m_id >= 0 && m_id < std::ssize(s_tasks));
    s_tasks[m_id] = this;
}


Task::Task(EntryPoint entryPoint, const void* args, size_t sizeArgs, const std::shared_ptr<PageTable>& pageTable)
:   Task(pageTable)
{
    // If args is an object, we want to copy it somewhere inside the new thread's context.
    // The top of the stack works just fine (for now?).
    if (sizeArgs > 0)
    {
        memcpy((void*)(this + 1), args, sizeArgs);
        args = this + 1;
    }

    ArchInit(entryPoint, args);
}


Task::~Task()
{
    // TODO: free all resources

    s_tasks[m_id] = nullptr;
}


void Task::Idle()
{
    // Set priority on this task
    auto task = cpu_get_data(task);
    task->m_priority = TaskPriority::Idle;

    for (;;)
    {
        //Log("#");

        // Verify that we have the lock
        assert(g_bigKernelLock.owner() == cpu_get_data(id));

        // TEMP: if there is any task to run, do not go idle
        // TODO: need better handling here, ideally the idle task doesn't get to run at all
        if (1)//sched_pending_work())
        {
            g_scheduler.Schedule();
        }

        // "else" is commented out for now, otherwise a CPU can get into an infinite Scheduler::Schedule() loop between two idle tasks.
        // The problem is that sched_pending_work() above sees an idle task and thinks there is work to do and switch to it.
        // That idle task in turn calls sched_pending_work() and see the previous idle task and switch to it. This creates a ping-pong
        // scheduling between the two idle tasks. What's worst, the kernel lock is never released and all the other CPUs are blocked.
        // The proper fix is a better scheduler and/or better handling of idle tasks. Task priorities could help as well.

        //else
        {
            g_bigKernelLock.unlock();
            interrupt_enable();

            // TODO: here we really want to halt, not pause... but we don't have a way to wakeup the halted CPU yet
            //cpu_halt();
            cpu_pause();

            interrupt_disable();
            g_bigKernelLock.lock();
        }
    }
}


void Task::Entry(Task* task, EntryPoint entryPoint, const void* args)
{
    assert(!interrupt_enabled());
    assert(g_bigKernelLock.owner() == cpu_get_data(id));

    //Log("Task::Entry(), id %d, entryPoint %p, args %p\n", task->id, entryPoint, args);

    entryPoint(task, args);

    assert(g_bigKernelLock.owner() == cpu_get_data(id));

    Log("Task %d exiting\n", task->m_id);

    // TODO: return task status code and not always 0
    g_scheduler.Die(0);
}


void Task::Wakeup()
{
    assert(IsBlocked());
    assert(m_queue != nullptr);

    m_queue->Wakeup(this);
}
