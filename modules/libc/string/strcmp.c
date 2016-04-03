#include <string.h>


int strcmp(const char* string1, const char* string2)
{
    while (*string1 && *string1 == *string2)
    {
        ++string1;
        ++string2;
    }

    return *(const unsigned char*)string1 - *(const unsigned char*)string2;
}
