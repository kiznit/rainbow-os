/*
    Copyright (c) 2015, Thierry Tremblay
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

#include "console.hpp"
#include "vga.hpp"

static int console_row;
static int console_column;
static uint8_t console_color;
static uint16_t* console_buffer;

void console_init()
{
    vga_hide_cursor();

    console_row = 0;
    console_column = 0;
    console_color = vga_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    console_buffer = VGA_MEMORY;
    uint16_t c = vga_make_char(' ', console_color);

    for (int y = 0; y != VGA_HEIGHT; ++y)
    {
        for (int x = 0; x != VGA_WIDTH; ++x)
        {
            const int index = y * VGA_WIDTH + x;
            console_buffer[index] = c;
        }
    }

    // Rainbow!
    console_buffer[0] = vga_make_char('R', VGA_COLOR_RED);
    console_buffer[1] = vga_make_char('a', VGA_COLOR_BROWN);
    console_buffer[2] = vga_make_char('i', VGA_COLOR_YELLOW);
    console_buffer[3] = vga_make_char('n', VGA_COLOR_LIGHT_GREEN);
    console_buffer[4] = vga_make_char('b', VGA_COLOR_CYAN);
    console_buffer[5] = vga_make_char('o', VGA_COLOR_LIGHT_BLUE);
    console_buffer[6] = vga_make_char('w', VGA_COLOR_MAGENTA);

    console_column = 8;
}


void console_putchar(char c)
{
    if (c == '\n')
    {
        console_column = 0;
        ++console_row;
    }
    else
    {
        const int index = console_row * VGA_WIDTH + console_column;
        console_buffer[index] = vga_make_char(c, console_color);
        if (++console_column == VGA_WIDTH)
        {
            console_column = 0;
            ++console_row;
        }
    }

    if (console_row == VGA_HEIGHT)
    {
        console_scroll();
        --console_row;
    }
}


void console_scroll()
{
    // Can't use memcpy, some hardware is limited to 16 bits read/write
    int i;
    for (i = 0; i != VGA_WIDTH*(VGA_HEIGHT-1); ++i)
    {
        VGA_MEMORY[i] = VGA_MEMORY[i+VGA_WIDTH];
    }

    uint16_t c = vga_make_char(' ', console_color);
    for ( ; i != VGA_WIDTH*VGA_HEIGHT; ++i)
    {
        VGA_MEMORY[i] = c;
    }
}
