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

#include <elf.h>
#include <stdlib.h>


void _fini();
void _init();
void _init_newlib();

int main(int argc, char** argv);


char** __environ;
long*  __auxv;
long   __aux[AT_COUNT];


int _start(long* p)
{
    // Arguments to main()
    int argc = p[0];
    char** argv = (void*)(p + 1);

    // Environment
    char** envp = argv + argc + 1;

    // ELF Auxiliary vector
    int count = 0;
    while (envp[count])
    {
        ++count;
    }

    long* auxv = (long*)(envp + count + 1);
    for (long* p = auxv; *p;)
    {
        const long type = *p++;
        const long value = *p++;

        if (type < AT_COUNT)
        {
            __aux[type] = value;
        }
    }

    __environ = envp;
    __auxv = auxv;

    // Initialize the C runtime
    _init_newlib();

    // Call global constructors and initialize C++ runtime
    _init();

    // Execute program
    const int status = main(argc, argv);

    // Call global destructors
    _fini();

    // Exit program
    exit(status);
}
