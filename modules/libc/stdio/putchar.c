#include <stdio.h>
#include <libc-internals.h>



int putchar(int c)
{
    char string[2] = { c, '\0' };
    __rainbow_print(string, 1);
    return (unsigned char)c;
}
