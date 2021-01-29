SOURCES := \
	acpi.cpp \
	console.cpp \
	elf.cpp \
	fatal.cpp \
	kernel.cpp \
	ipc.cpp \
	mutex.cpp \
	pagetable.cpp \
	pmm.cpp \
	readyqueue.cpp \
	reent.cpp \
	scheduler.cpp \
	spinlock.cpp \
	syscall.cpp \
	task.cpp \
	usermode.cpp \
	vdso.cpp \
	vmm.cpp \
	waitqueue.cpp \
	runtime/malloc.cpp \
	runtime/newlib.cpp \
	runtime/pthread.cpp \
	runtime/syscalls.cpp


ifdef X86
MODULES := x86
endif
