SOURCES += \
	x86/interrupt.cpp \
	x86/pic.cpp \
	x86/timer.cpp \
	x86/$(ARCH)/cpu.cpp \
	x86/$(ARCH)/entry.S \
	x86/$(ARCH)/interrupt.S \
	x86/$(ARCH)/thread.S \


# Fix linker script
LDSCRIPT := $(SRCDIR)/x86/$(ARCH)/kernel.lds
