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

#include <rainbow/rainbow.h>


extern "C" size_t strlen(const char* string)
{
    size_t length = 0;
    while (*string++)
    {
        ++length;
    }

    return length;
}


static void Log(const char* text)
{
    char reply[64];
    ipc_call(1, text, strlen(text)+1, reply, sizeof(reply));
}


static int thread_function(void* text)
{
    for(;;)
    {
        Log((char*)text);
    }
}


extern "C" void _start()
{
    const auto stack_size = 65536;
    char* stack1 = (char*) mmap((void*)0xC0000000, stack_size);
    char* stack2 = (char*) mmap((void*)(0xC0000000 + stack_size), stack_size);

    spawn(thread_function, "1", 0, stack1 + stack_size, stack_size);
    spawn(thread_function, "2", 0, stack2 + stack_size, stack_size);

    for(;;)
    {
        Log("*");
    }
}
