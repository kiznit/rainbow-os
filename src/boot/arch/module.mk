SOURCES :=

ifdef X86
	SOURCES += \
		x86/check.cpp \
		x86/vmm.cpp
else
	SOURCES += \
		$(ARCH)/check.cpp \
		$(ARCH)/vmm.cpp
endif
