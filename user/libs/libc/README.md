libc
====

Currently we are using newlib in user space. But this is enough, so here we add more stuff (ex: pthread, malloc, CRTx support, etc).

Eventually we would like to ditch newlib and have our own libc implementation.


TODO
====

- Remove all dependencies on libposix. Right now we can't easily do that because we are using newlib and newlib is using posix system calls. Once we ditch newlib, we can implement our own libc that makes direct system calls to the OS without going through the posix API.
