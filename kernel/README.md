Rainbow Kernel
==============


Interrupts
----------

When entering the kernel (system calls, interrupts or CPU exceptions/faults), interrupts are disabled. This means that all kernel code
runs with interrupts disabled. This simplifies things.


IPC
---

### Overview

* Synchronous message based IPC (inspired from L4 / QNX).
* Aiming for high-performance using an implementation that looks somewhat like L4.


### Implementation

Current implementation works and might need some optimizations around scheduling. See <rainbow/ipc.h> for current API.


Task
----

Each task has its own kernel stack. Both the TCB (Task) and stack are allocated together in the same memory page. This saves memory and remove the need for a kernel heap to allocate TCBs.


Floating point
--------------

There is no floating point allowed (including SSE) inside the kernel. This is done for efficiency purposes.


VDSO (Virtual Dynamic Shared Object)
------------------------------------

The idea is taken directly from Linux. The kernel exposes a virtual DSO to each task. This DSO provides entry points for things like system calls.

At time of writing, VDSO are not implemented as such. The current implementation is more akin to L4's Kernel Interface Page (KIP).


### TODO

* IPC: Reserve SYSENTER / SYSCALL for IPCs and use interrupts for other system calls. L4 does that for ia32.
The idea is to maximize the performances of IPCs. Presumably there aren't many system calls and/or they are seldom used.

* IPC: Use IPC endpoints (ports) instead of task IDs to identify services / callers.

- IPC: investigate using capabilities (EROS IPC).

* IPC: Implement virtual registers in user space (and not inside the kernel's TCB). Presumably we want to start defining a user-space TCB (UTCB).
This could be part of a kernel interface page (KIP) / VDSO.

* IPC: Figure out wait queues (right now there are two inside the TCB, it doesn't seem optimal).

* IPC: Lazy scheduling (switch Task on IPC calls without entering the kernel's scheduler) - see L4.

* IPC: provide optional non-blocking send and receive (timeout of 0). The concern here is DOS attacks from a client calling a server and not collecting the reply.

* IPC: synchronous IPC doesn't scale well for SMP, provide asynchronous IPC

* Compile kernel with -O3

* Figure out how I want to handle system calls... have a rainbow library?

* IPC through syscall, every other system call through its own interrupt number (?)
