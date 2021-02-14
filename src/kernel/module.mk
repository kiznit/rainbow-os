SOURCES := \
	acpi.cpp \
	console.cpp \
	elf.cpp \
	fatal.cpp \
	futex.cpp \
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
	runtime/crt.cpp \
	runtime/malloc.cpp \
	runtime/newlib.cpp


ifdef X86
MODULES := x86
endif
