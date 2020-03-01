SOURCES := \
	console.cpp \
	elf.cpp \
	heap.cpp \
	kernel.cpp \
	ipc.cpp \
	pmm.cpp \
	scheduler.cpp \
	semaphore.cpp \
	spinlock.cpp \
	thread.cpp \
	usermode.cpp \
	vmm.cpp


ifdef X86
MODULES := x86
endif
