SOURCES := \
	acpi.cpp \
	console.cpp \
	elf.cpp \
	fatal.cpp \
	kernel.cpp \
	ipc.cpp \
	pagetable.cpp \
	pmm.cpp \
	reent.cpp \
	scheduler.cpp \
	spinlock.cpp \
	syscall.cpp \
	task.cpp \
	usermode.cpp \
	vdso.cpp \
	vmm.cpp \
	waitqueue.cpp \
	libc/malloc.cpp \
	libc/newlib_lock.cpp \
	libc/newlib_syscall.cpp


ifdef X86
MODULES := x86
endif
