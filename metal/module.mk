SOURCES := \
	crt.cpp \
	log.cpp

ifdef X86
	SOURCES += \
		x86/cpuid.cpp
endif