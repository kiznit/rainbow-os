SOURCES := \
	apic.cpp \
	cpu.cpp \
	exceptions.cpp \
	interrupt.cpp \
	machine.cpp \
	pic.cpp \
	pit.cpp \
	pmtimer.cpp \
	smp.cpp \
	$(ARCH)/cpu.cpp \
	$(ARCH)/entry.S \
	$(ARCH)/pagetable.cpp \
	$(ARCH)/smp.S \
	$(ARCH)/start.S \
	$(ARCH)/task.cpp \
	$(ARCH)/task.S \
	$(ARCH)/usermode.S
