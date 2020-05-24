Rainbow Kernel
==============


IPC
---

### Overview

* Synchronous message based IPC (inspired from QNX)
* Aiming for high-performance using an implementation that looks somewhat like L4Ka (using virtual registers)


### Implementation

Current implementation is very slow and inefficient. Much work to be done here.

Current API:

    * ipc_call() - blocking send message, will unblock when service returns with ipc_reply()
    * ipc_reply() - return data to the caller and unblock it
    * ipc_reply_and_wait() - ipc_reply() + ipc_wait() in one call
    * ipc_wait() - wait for a caller


### TODO

* Reserve SYSENTER / SYSCALL for IPCs and use interrupts for other system calls. L4Ka does that for ia32.
The idea is to maximize the performances of IPCs. Presumably there aren't many system calls and/or they are seldom used.

* Use IPC endpoints (ports) instead of task IDs to identify services / callers.

* Implement virtual registers in user space (and not inside the kernel's TCB). Presumably we want to start defining a user-space TCB (UTCB).
This could be part of a kernel interface page (KIP) / VDSO.

* Figure out wait queues (right now there are two inside the TCB, it doesn't seem optimal)

* Lazy scheduling (switch Task on IPC calls without entering the kernel's scheduler) - see L4


