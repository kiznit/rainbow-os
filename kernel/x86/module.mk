SOURCES := \
	exceptions.cpp \
	interrupt.cpp \
	machine.cpp \
	pic.cpp \
	pit.cpp \
	$(ARCH)/cpu.cpp \
	$(ARCH)/entry.S \
	$(ARCH)/pagetable.cpp \
	$(ARCH)/start.S \
	$(ARCH)/task.cpp \
	$(ARCH)/task.S \
	$(ARCH)/usermode.S \
	$(ARCH)/vmm.cpp
