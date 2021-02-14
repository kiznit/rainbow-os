SOURCES := \
	crt.cpp \
	boot.cpp \
	display.cpp \
	elfloader.cpp \
	memory.cpp \
	runtime/malloc.cpp \
	runtime/newlib.cpp


MODULES := arch machine/$(MACHINE)
