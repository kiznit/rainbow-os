#include <string.h>


void* memcpy(void* dest, const void* src, size_t n)
{
    unsigned char* p = dest;
    const unsigned char* q = src;

    while (n--)
        *p++ = *q++;

    return dest;
}
