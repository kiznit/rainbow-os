#ifndef _INCLUDED_LIBC_INTERNALS_H
#define _INCLUDED_LIBC_INTERNALS_H

#ifdef __cplusplus
extern "C" {
#endif

void __rainbow_putc(unsigned char c);
int __rainbow_print(const char* string);


#ifdef __cplusplus
}
#endif

#endif
