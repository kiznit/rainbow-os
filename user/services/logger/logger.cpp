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

#include <stdio.h>
#include <rainbow/rainbow.h>


const char* ids[] = {
    "0\n",
    "1\n",
    "2\n",
    "3\n",
    "4\n",
    "5\n",
    "6\n",
    "7\n",
    "8\n",
    "9\n"
};


extern "C" int main()
{
    setbuf(stdout, NULL);

    printf("THIS IS LOGGER\n");

    if (1)
    {
        char buffer[256];
        int caller = ipc_wait(buffer, sizeof(buffer));

        while (caller >= 0)
        {
            fputs(buffer, stdout);
            caller = ipc_reply_and_wait(caller, nullptr, 0, buffer, sizeof(buffer));
        }
    }
    else
    {
        char buffer[256];
        while (1)
        {
            ipc_wait(buffer, sizeof(buffer));
            fputs(buffer, stdout);
        }
    }

    return 0;
}
