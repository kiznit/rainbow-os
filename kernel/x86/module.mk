SOURCES := \
	interrupt.cpp \
	machine.cpp \
	pic.cpp \
	pit.cpp \
	$(ARCH)/cpu.cpp \
	$(ARCH)/entry.S \
	$(ARCH)/interrupt.S \
	$(ARCH)/pagetable.cpp \
	$(ARCH)/thread.cpp \
	$(ARCH)/thread.S \
	$(ARCH)/usermode.S \
	$(ARCH)/vmm.cpp
