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

#include <string.h>
#include <sys/mman.h>
#include <rainbow/rainbow.h>


static void Log(const char* text)
{
    if (1)
    {
        char reply[64];
        ipc_call(51, text, strlen(text)+1, reply, sizeof(reply));
    }
    else
    {
        ipc_send(51, text, strlen(text)+1);
    }
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
    const auto STACK_SIZE = 65536;
    char* stack1 = (char*)mmap(nullptr, STACK_SIZE, PROT_WRITE, MAP_ANONYMOUS, -1, 0);
    char* stack2 = (char*)mmap(nullptr, STACK_SIZE, PROT_WRITE, MAP_ANONYMOUS, -1, 0);

    spawn(thread_function, "1", 0, stack1 + STACK_SIZE, STACK_SIZE);
    spawn(thread_function, "2", 0, stack2 + STACK_SIZE, STACK_SIZE);

    for(;;)
    {
        Log("*");
    }
}
