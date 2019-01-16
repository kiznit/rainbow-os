SOURCES := \
	console.cpp \
	kernel.cpp \
	pmm.cpp \
	thread.cpp


ifdef X86
MODULES := x86
endif
