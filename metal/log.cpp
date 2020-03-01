/*
    Copyright (c) 2018, Thierry Tremblay
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

#include "log.hpp"
#include <stddef.h>
#include <stdint.h>
#include "arch.hpp"
#include "console.hpp"


IConsole* g_console;


static const char digits[] = "0123456789abcdef";


void Log(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    Log(format, args);
    va_end(args);
}


void Log(const char* format, va_list args)
{
    char buffer[500];                   // Buffer to hold the formatted string
    size_t size = sizeof(buffer) - 1;   // Minus 1 to account for terminating '\0'
    char* p = buffer;                   // Write pointer
    char convert[64];                   // Temporary buffer
    int base;                           // Base for converting numbers
    int precision;                      // How many digits to use to print a number
    uint64_t value;                     // Numeric value for conversions
    char c;                             // Character being processed

#define PUTCH(ch) do {      \
    if ( size-- <= 0 )      \
        goto Done;          \
    *p++ = (ch);            \
    } while (0)

    while (size > 0)
    {
        while ((c = *format++) != '%')
        {
            if (size-- <= 0 || c == '\0')
            {
                goto Done;
            }
            *p++ = c;
        }

        base = 10;
        precision = -1;

        switch (c = *format++)
        {
            case 'c':
            {
                value = va_arg(args, int);
                PUTCH(value);
                break;
            }

            case 'd':
            {
                value = va_arg(args, int);
                if ((int64_t) value < 0)
                {
                    PUTCH('-');
                    value = -value;
                }
                goto PrintNumber;
            }

            case 's':
            {
                const char* s = va_arg(args, const char*);
                while (*s) PUTCH(*s++);
                break;
            }

            case 'x':
            {
                value = va_arg(args, unsigned);
                base = 16;
                precision = sizeof(unsigned)*2;
                goto PrintNumber;
            }

            case 'X':
            {
                value = va_arg(args, uint64_t);
                base = 16;
                precision = sizeof(uint64_t)*2;
                goto PrintNumber;
            }

            case 'p':
            {
                value = va_arg(args, uintptr_t);
                base = 16;
                precision = sizeof(uintptr_t)*2;

            PrintNumber:

                int numDigits = 0;

                do
                {
                    convert[numDigits++] = digits[value % base];
                    value = value / base;
                } while (value > 0);

                for (int i = precision - numDigits; i > 0; --i)
                {
                    PUTCH('0');
                }

                while (numDigits > 0)
                {
                    PUTCH(convert[--numDigits]);
                }
                break;
            }

            default:
            {
                PUTCH('%');
                PUTCH(c);
                break;
            }
        }
    }

Done:
    *p = '\0';

    if (g_console)
    {
        const auto enableInterrupts = interrupt_enabled();

        interrupt_disable();

        g_console->Print(buffer);

        if (enableInterrupts)
        {
            // TODO: I don't like having this call in bootloaders...
            interrupt_enable();
        }
    }
}



void Fatal(const char* format, ...)
{
    // TODO: here we really want to stop all threads! (i.e. panic!) (kernel, but not bootloaders)

    // TODO: I don't like having this call in bootloaders...
    interrupt_disable();

    Log("\nFATAL: ");

    va_list args;
    va_start(args, format);
    Log(format, args);
    va_end(args);

    for (;;);
}
