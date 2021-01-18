/*
    Copyright (c) 2021, Thierry Tremblay
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

#include <rainbow/syscall.h>
#include <unistd.h>

void _fini();

/*
    TODO: there are differences between exit(), _exit() and _Exit(). This needs to be properly handled.

    exit()  --> does cleanup (atexit, flush streams, call C++ destructors)
    _Exit() --> doesn't cleanup (no atexit, streams not flushed, no C++ destructors)
    _exit() --> not standard in either C or C++. POSIX says it's a synonym for _Exit() and this is what newlib does.

    "On Linux they are equivalent. On OS X (BSD), however, The _Exit() function terminates without calling the functions
     registered with the atexit(3) function, and may or may not perform the other actions listed that exit() (without
     underscore) does which are flushing open output streams, closing open streams and unlinking temporary files created
     using tmpfile(3). After this both exit() and _Exit() calls _exit() to terminate the process."

    "Program termination is addressed in C++2003 section [3.6.3]. It says that static objects are destructed implicitly
     when main() returns and when exit() is called. It also says that such objects are NOT destructed when abort() is
     called. _exit() isn't addressed in the C++ 2003 standard, but the fact that it is meant to bypass language-specific
     cleanup is described in the POSIX documentation. That effect is further substantiated by what is stated and by what
     is NOT stated in the C++ standard.""
*/

void _exit(int status)
{
    // Call destructors - TODO: this shouldn't be, see big block of text above
    _fini();

    // Exit program
    syscall1(SYSCALL_EXIT, status);

    // Should never be reached
    for (;;);
}
