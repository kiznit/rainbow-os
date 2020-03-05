SOURCES := \
	interrupt.cpp \
	machine.cpp \
	pic.cpp \
	pit.cpp \
	$(ARCH)/cpu.cpp \
	$(ARCH)/entry.S \
	$(ARCH)/interrupt.S \
	$(ARCH)/pagetable.cpp \
	$(ARCH)/syscall.cpp \
	$(ARCH)/task.cpp \
	$(ARCH)/task.S \
	$(ARCH)/usermode.S \
	$(ARCH)/vmm.cpp
