SOURCES := \
	console.cpp \
	elf.cpp \
	fatal.cpp \
	heap.cpp \
	kernel.cpp \
	ipc.cpp \
	pmm.cpp \
	scheduler.cpp \
	semaphore.cpp \
	spinlock.cpp \
	syscall.cpp \
	task.cpp \
	usermode.cpp \
	vdso.cpp \
	vmm.cpp


ifdef X86
MODULES := x86
endif
