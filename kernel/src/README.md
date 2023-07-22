Rainbow OS Kernel
=================


std::shared_ptr<>
-----------------

Watch out about using std::shared_ptr<> on the stack. If a task is killed while any std::shared_ptr<> exists on the stack, these will never get released. It is likely fine to have have them in non-preemptable contexts if we can guarantee that the task won't be killed with active shared pointers on the stack.

