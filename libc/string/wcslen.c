#include <string.h>


size_t wcslen(const wchar_t* string)
{
    size_t count = 0;

    for (const wchar_t* p = string; *p; ++p)
    {
        ++count;
    }

    return count;
}
