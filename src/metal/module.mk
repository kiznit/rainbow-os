INCLUDES += $(TOPDIR)/metal/include $(TOPDIR)/metal/include/c++

SOURCES := \
	src/c++/new.cpp \
	src/c++/shared_ptr.cpp \
	src/libc/abort.cpp \
	src/libc/assert.cpp \
	src/libc/memcpy.cpp \
	src/libc/memmove.cpp \
	src/libc/memset.cpp \
	src/libc/printf.cpp \
	src/libc/strcmp.cpp \
	src/libc/strcpy.cpp \
	src/libc/strlen.cpp \
	src/runtime/__cxa_atexit.cpp \
	src/runtime/crti.c \
	src/runtime/crtn.c

ifdef X86
	SOURCES += \
		src/arch/x86/cpu.cpp \
		src/arch/x86/cpuid.cpp
endif
