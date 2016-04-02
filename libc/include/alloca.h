#ifndef _RAINBOW_LIBC_ALLOCA_H
#define _RAINBOW_LIBC_ALLOCA_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef  __GNUC__
#define alloca __builtin_alloca
#endif

#ifdef __cplusplus
}
#endif


#endif
