SOURCES := \
	crt.cpp \
	boot.cpp \
	elfloader.cpp \
	memory.cpp


MODULES := arch machine/$(MACHINE)
