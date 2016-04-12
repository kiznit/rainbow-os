#include <assert.h>
#include <stdio.h>
#include <stdlib.h>



void _assert(const char* expression, const char* file, int line, const char* function)
{
    //todo: this should print to stderr, not stdout

    printf("Debug Assertion Failed:\n");
    printf("    File      : %s\n", file);
    printf("    Function  : %s\n", function);
    printf("    Line      : %d\n", line);
    printf("    Expression: %s\n", expression);

    abort();
}
