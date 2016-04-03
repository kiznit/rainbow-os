#ifndef _RAINBOW_LIBC_STRING_H
#define _RAINBOW_LIBC_STRING_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int memcmp(const void*, const void*, size_t);
void* memcpy(void*, const void*, size_t);
void* memmove(void*, const void*, size_t);
void* memset(void*, int, size_t);

//void* memchr(const void* memory, int character, size_t size);

char* strcat(char*, const char*);
//char* strncat(char*, const char*, size_t);

int strcmp(const char*, const char *);

char* strncpy(char*, const char*, size_t);

char* strchr(const char* string, int character);
size_t strlen(const char* string);


size_t wcslen(const wchar_t* string);


#ifdef __cplusplus
}
#endif

#endif
