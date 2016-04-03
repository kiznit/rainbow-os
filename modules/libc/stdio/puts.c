#include <stdio.h>
#include <string.h>
#include <libc-internals.h>



int puts(const char* string)
{
    int result = __rainbow_print(string, strlen(string));
    putchar('\n');
    return result;
}
