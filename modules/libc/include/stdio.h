#ifndef _RAINBOW_LIBC_STDIO_H
#define _RAINBOW_LIBC_STDIO_H

#include <stdarg.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EOF (-1)


int getchar(void);

int printf(const char* format, ...);
int snprintf(char* buffer, size_t size, const char* format, ...);
int vprintf(const char* format, va_list args);
int vsnprintf(char* buffer, size_t size, const char* format, va_list args);

int putchar(int c);
int puts(const char* string);



int _libc_print(const char* string, size_t length);


#ifdef __cplusplus
}
#endif

#endif
