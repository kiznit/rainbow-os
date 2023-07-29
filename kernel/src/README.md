Rainbow OS Kernel
=================


std::shared_ptr<> and other smart pointers
------------------------------------------

It is not safe to use smart pointers like std::shared_ptr<> on the stack. If a task gets killed while there is any std::shared_ptr<> on its stack, these will never get released. It is likely fine to have have them in non-preemptable contexts if we can guarantee that the task won't be killed with active shared pointers on the stack, but that doesn't seem very useful.

The same applies to any other type of smart pointer / automatic resource management... Using them on the stack is simply not safe within the kernel.

That said, using std::shared_ptr<> as member of managed resources can be made safe as they are guaranteed to be released when a task dies. The real problem with the stack is that it is unmanaged. If we had a way to walk the stack back and call destructors when a task is destroyed, we could consider using smart pointers on the stack. Essentially we would have to enable/implement exception handling and the last time I tried this it was not a plesant experience.
