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
#include <kernel/x86/selectors.hpp>
#include <rainbow/usertask.h>


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


Task::Task(const std::shared_ptr<PageTable>& pageTable)
:   m_id(s_nextTaskId++),
    m_state(STATE_INIT),
    m_priority(PRIORITY_NORMAL),
    m_pageTable(pageTable)
{
    assert(!!m_pageTable);

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

    s_tasks.erase(m_id);
}


void Task::Idle()
{
    // Set priority on this task
    auto task = cpu_get_data(task);
    task->m_priority = Task::PRIORITY_IDLE;

    for (;;)
    {
        Log("#");

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

    int status = 0;

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

        status = -1;
    }

    assert(g_bigKernelLock.owner() == cpu_get_data(id));

    Log("Task %d exiting\n", task->m_id);

    sched_die(status);
}


void Task::InitUserTaskAndTls()
{
    auto tlsSize = align_up(m_tlsSize, alignof(UserTask));
    auto totalSize = align_up(tlsSize + sizeof(UserTask), MEMORY_PAGE_SIZE);

    m_userTls = m_pageTable->AllocatePages(totalSize >> MEMORY_PAGE_SHIFT);
    memcpy(m_userTls, m_tlsTemplate, m_tlsTemplateSize);

    UserTask* userTask = (UserTask*)advance_pointer(m_userTls, tlsSize);
    userTask->self = userTask;
    userTask->id = m_id;

    m_userTask = userTask;

    if (cpu_get_data(task) == this)
    {
#if defined(__i386__)
        auto gdt = cpu_get_data(gdt);
        gdt[7].SetUserData32((uintptr_t)m_userTask, sizeof(UserTask));    // Update GDT entry
        asm volatile ("movl %0, %%gs\n" : : "r" (GDT_TLS) : "memory" );   // Reload GS
#elif defined(__x86_64__)
        // We need to set MSR_FS_BASE here because TLS wasn't intiialized at time of task switch
        x86_write_msr(MSR_FS_BASE, (uintptr_t)m_userTask);
#endif
    }
}
