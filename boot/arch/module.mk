SOURCES :=

ifdef X86
	SOURCES += \
		x86/check.cpp \
		x86/vmm.cpp
endif
