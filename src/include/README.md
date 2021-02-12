C++ Hacks
=========

Even though we do not link libstdc++ with the kernel (or bootloader), we still are using lot of useful stuff like smart pointers.

But using the libstdc++ headers as-is doesn't work on x86_64 with disabled floating point.

So this directory (and bits) contain "standard C++" headers that are basically hacks to allow us to use what we want/need and ignore the parts that don't compile.

It's not clear to me at this point if I want to keep things like this, implement my own version of these headers or ditch them entirely and remove any references to "std::" from the code.
