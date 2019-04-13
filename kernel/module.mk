SOURCES := \
	console.cpp \
	kernel.cpp \
	pmm.cpp \
	scheduler.cpp \
	thread.cpp


ifdef X86
MODULES := x86
endif
