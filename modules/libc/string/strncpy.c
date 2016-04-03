#include <string.h>


char* strncpy(char* destination, const char* source, size_t count)
{
    size_t i;

    for (i = 0; i != count && source[i] != '\0'; ++i)
        destination[i] = source[i];

    for ( ; i != count; ++i)
        destination[i] = '\0';

    return destination;
}
