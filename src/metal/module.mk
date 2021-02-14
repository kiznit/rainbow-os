INCLUDES += $(TOPDIR)/metal/include $(TOPDIR)/metal/include/c++

SOURCES := \
	src/arch/$(ARCH)/crti.S \
	src/arch/$(ARCH)/crtn.S \
	src/c++/__cxa_atexit.cpp \
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
	src/libc/strlen.cpp

ifdef X86
	SOURCES += \
		src/arch/x86/cpu.cpp \
		src/arch/x86/cpuid.cpp
endif
