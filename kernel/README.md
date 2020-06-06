Rainbow Kernel
==============


Interrupts
----------

When entering the kernel (system calls, interrupts or CPU exceptions/faults), interrupts are disabled. This means that all kernel code
runs with interrupts disabled. This simplifies things.


IPC
---

### Overview

* Synchronous message based IPC (inspired from QNX)
* Aiming for high-performance using an implementation that looks somewhat like L4Ka (using virtual registers)


### Implementation

Current implementation is very slow and inefficient. Much work to be done here.

Current API:

    * ipc_call() - blocking send message, will unblock when service returns with ipc_reply()
    * ipc_reply_and_wait() - ipc_reply() + ipc_wait() in one call
    * ipc_wait() - wait for a caller


Task
----

Each task has its own kernel stack. Both the TCB (Task) and stack are allocated together in the same memory page. This saves memory and remove the need for a kernel heap to allocate TCBs.


VDSO (Virtual Dynamic Shared Object)
------------------------------------

The idea is taken directly from Linux. The kernel exposes a virtual DSO to each task. This DSO provides entry points for things like system calls.

At time of writing, VDSO are not implemented as such. The current implementation is more akin to L4Re's Kernel Interface Page (KIP).


### TODO

* Reserve SYSENTER / SYSCALL for IPCs and use interrupts for other system calls. L4Ka does that for ia32.
The idea is to maximize the performances of IPCs. Presumably there aren't many system calls and/or they are seldom used.

* Use IPC endpoints (ports) instead of task IDs to identify services / callers.

* Implement virtual registers in user space (and not inside the kernel's TCB). Presumably we want to start defining a user-space TCB (UTCB).
This could be part of a kernel interface page (KIP) / VDSO.

* Figure out wait queues (right now there are two inside the TCB, it doesn't seem optimal)

* Lazy scheduling (switch Task on IPC calls without entering the kernel's scheduler) - see L4
