#ifndef _RAINBOW_LIBC_ERRNO_H
#define _RAINBOW_LIBC_ERRNO_H

#ifdef __cplusplus
extern "C" {
#endif



#define EINVAL 21
#define ENOMEM 23


//todo: this needs to be per-thread
extern int errno;



#ifdef __cplusplus
}
#endif

#endif
