SOURCES := \
	boot.cpp \
	display.cpp \
	elfloader.cpp \
	fatal.cpp \
	memory.cpp \
	runtime/crt.cpp \
	runtime/newlib.cpp


MODULES := arch machine/$(MACHINE)
