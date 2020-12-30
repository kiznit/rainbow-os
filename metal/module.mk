SOURCES := \
	$(ARCH)/crti.S \
	$(ARCH)/crtn.S

ifdef X86
	SOURCES += \
		x86/cpu.cpp \
		x86/cpuid.cpp
endif
