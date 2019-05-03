SOURCES := \
	console.cpp \
	heap.cpp \
	kernel.cpp \
	ipc.cpp \
	memorymap.cpp \
	pmm.cpp \
	scheduler.cpp \
	semaphore.cpp \
	spinlock.cpp \
	thread.cpp


ifdef X86
MODULES := x86
endif
