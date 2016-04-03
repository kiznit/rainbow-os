#include <stdio.h>
#include <libc-internals.h>



int puts(const char* string)
{
    int result = __rainbow_print(string);
    __rainbow_putc('\n');
    return result;
}
