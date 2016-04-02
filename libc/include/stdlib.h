#ifndef _RAINBOW_LIBC_STDLIB_H
#define _RAINBOW_LIBC_STDLIB_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


void* malloc(size_t size);
void free(void* memory);


#ifdef __cplusplus
}
#endif

#endif
