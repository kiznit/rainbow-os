#include <stdio.h>
#include <string.h>



int puts(const char* string)
{
    int result = _libc_print(string, strlen(string));
    if (result < 0)
        return result;

    result = putchar('\n');
    if (result < 0)
        return result;

    return result;
}
