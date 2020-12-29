SOURCES := \
	crt.cpp \
	boot.cpp \
	display.cpp \
	elfloader.cpp \
	memory.cpp \
	libc/malloc.cpp \
	libc/newlib.cpp


MODULES := arch machine/$(MACHINE)
