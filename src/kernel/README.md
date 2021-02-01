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

Current implementation works and might need some optimizations around scheduling. See \<rainbow/ipc.h\> for current API.


Task
----

Each task has its own kernel stack. Both the TCB (Task) and stack are allocated together in the same memory page. This saves memory and remove the need for a kernel heap to allocate TCBs.


Reentrancy
----------

There is currently a big kernel lock that is held when entering the kernel from anywhere. Kernel code also runs with interrupts disabled.

Nevertheless, reentrancy is still a thing as kernel code can trigger CPU exceptions (traps, faults, ...).

We need to ensure that the libraries we use can handle reentrancy properly:

- libgcc : don't know at this point, the main concern is C++ exception handling and stack unwinding.
- newlib : we handle this in reent.cpp by creating a new newlib context (struct \_reent).
- libstdc++ : don't know at this point, most of it is probably safe, but this needs to be verified.

Alternatively, we could ensure that we don't use libgcc or libstdc++ from inside CPU exception handlers.


Floating point
--------------

Because I really wanted to have standard c++ inside the kernel, we have to enable floating point units in the kernel. This is true for at least x86_64 as SSE registers are part of the ABI to pass floating point values around.

I thought I could get away with disabling floating point code and still have a viable C++ library. I patched / configured GCC and libgcc to not have any floating point code. This worked well. I then patched / configured newlib to also not have any floating point code, which meant amonst other things to completely remove libm. Life was good. I started using C++ headers and hitting some errors. For example \<limits\> has some templates and functions using floating point (i.e. numeric_limits\<float\>). Including \<mutex\> for std::lock_guard\<\> means indirectly including \<bits/std_abs.h\> which defines std::abs(float). I kept on patching libstdc++ as I was encountering errors. At some point I ran into \<algorithm\> (required for std::ranges) that includes code for hash tables and std::unordered_map\<\>. This code requires floating point code. Do I really want to start removing all the hash table code, including std::unordered_map\<\>? Where does this end? Ending up with a crippled standard C++ library is not great and was not the goal in the first place.

Another think that I wasn't exactly thrilled with is that the more patches are being applied, the more difficult it will be to maintain the scripts to build the cross compilers. It also makes it more and more difficult to migrate to another compiler and another C or C++ library. Even upgrading gcc / newlib to new versions would be significant work. This is not a good thing.

So what does this mean... Well it means that the kernel will have to do a lot more save/restore of the FPU state. This is always required when swapping between threads, but now it is also required when doing other type of context switches like entering trap/fault handlers, nested interrupts handlers (when we support this) and possibly other cases that don't come to mind right now. Some platforms might not require these extra save/restore of the FPU state, but I am not sure at this point. Even ia32 will require these save/restore if it uses any std::unordered_map\<\> or other code that uses floating point code.

With the above said, I do believe we can try to optimize things a bit by only lazily saving/restoring the FPU state in some cases.

Current implementation is this:

1) User space FPU state is saved to the task when entering the kernel (syscalls, interrupts, exceptions)
2) User space FPU state is restored from the task when returning from kernel (syscalls, interrupts, exceptions)
3) Kernel space FPU state is saved/restored when handling reentrancy, see "reent.cpp" for details.



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
