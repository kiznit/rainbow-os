SOURCES := \
	acpi.cpp \
	console.cpp \
	elf.cpp \
	fatal.cpp \
	kernel.cpp \
	ipc.cpp \
	pmm.cpp \
	scheduler.cpp \
	spinlock.cpp \
	syscall.cpp \
	task.cpp \
	usermode.cpp \
	vdso.cpp \
	vmm.cpp \
	libc/malloc.cpp \
	libc/newlib.cpp


ifdef X86
MODULES := x86
endif
