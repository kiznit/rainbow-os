#include <stdio.h>



int putchar(int c)
{
    char string[2] = { c, '\0' };

    int result = _libc_print(string, 1);
    if (result < 0)
        return result;

    return (unsigned char)c;
}
