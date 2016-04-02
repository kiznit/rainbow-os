#include <string.h>


size_t strlen(const char* string)
{
    size_t count = 0;

    for (const char* p = string; *p; ++p)
    {
        ++count;
    }

    return count;
}
