INCLUDES += $(TOPDIR)/metal/include $(TOPDIR)/metal/include/c++

SOURCES := \
	src/$(ARCH)/crti.S \
	src/$(ARCH)/crtn.S \
	src/c++/new.cpp \
	src/c++/shared_ptr.cpp

ifdef X86
	SOURCES += \
		src/x86/cpu.cpp \
		src/x86/cpuid.cpp
endif
