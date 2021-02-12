SOURCES := \
	crt.cpp \
	boot.cpp \
	display.cpp \
	elfloader.cpp \
	memory.cpp \
	runtime/malloc.cpp \
	runtime/new.cpp \
	runtime/newlib.cpp \
	runtime/syscalls.cpp


MODULES := arch machine/$(MACHINE)
