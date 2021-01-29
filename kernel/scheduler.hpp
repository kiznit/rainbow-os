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

#ifndef _RAINBOW_KERNEL_SCHEDULER_HPP
#define _RAINBOW_KERNEL_SCHEDULER_HPP

#include <cstdint>
#include <memory>
#include "taskdefs.hpp"

class Task;
class WaitQueue;


extern bool sched_should_switch;


// Initialize the scheduler
void sched_initialize();

// Add a task to this scheduler
void sched_add_task(std::unique_ptr<Task>&& task);

// Schedule a new task for execution
void sched_schedule();

// Switch execution to the specified task
void sched_switch(std::unique_ptr<Task>&& newTask);

// Sleep for 'x' nanoseconds (or more, no guarantees)
void sched_sleep(uint64_t durationNs);

// Sleep until the specified clock time (in ns)
void sched_sleep_until(uint64_t clockTimeNs);

// Yield the CPU to another task
extern "C" int sched_yield();

// Kill the current task - TODO: weird API!
void sched_die(int status) __attribute__((noreturn));


#endif
