# Copyright (c) 2020, Thierry Tremblay
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Make sure we are using bash
SHELL := /bin/bash

.SECONDARY:

.SUFFIXES:
.SUFFIXES: .c .cpp .h. .hpp .s .S .o


###############################################################################
#
# Build tools
#
###############################################################################

HOSTCC      = $(prefix)gcc
CC          = $(prefix)$(CROSS_COMPILE)gcc
AS          = $(prefix)$(CROSS_COMPILE)as
LD          = $(prefix)$(CROSS_COMPILE)ld
AR          = $(prefix)$(CROSS_COMPILE)ar
RANLIB      = $(prefix)$(CROSS_COMPILE)ranlib
OBJCOPY     = $(prefix)$(CROSS_COMPILE)objcopy


###############################################################################
#
# Host / target
#
###############################################################################

# Arch of the host machine
HOSTARCH ?= $(shell $(HOSTCC) -dumpmachine | cut -f1 -d- | sed -e s,i[3456789]86,ia32, -e 's,armv7.*,arm,')

# Some compilers report amd64 instead of x86_64
ifeq ($(HOSTARCH),amd64)
	override HOSTARCH = x86_64
endif

# Select the right architecture
ifeq ($(ARCH),)
	ifneq ($(CROSS_COMPILE),)
		# We have a cross compiler to use, figure out the ARCH from it
		CCARCH ?= $(shell $(CC) -dumpmachine | cut -f1 -d- | sed -e s,i[3456789]86,ia32, -e 's,armv7.*,arm,')
		ifeq ($(CCARCH),amd64)
			override CCARCH = x86_64
		endif
		ARCH = $(CCARCH)
	else
		# No arch or cross compiler specified, use host arch
		ARCH = $(HOSTARCH)
	endif
else ifeq ($(ARCH),amd64)
	override ARCH = x86_64
endif

# Select the right cross comnpiler
ifeq ($(CROSS_COMPILE),)
	ifeq ($(ARCH),ia32)
		CROSS_COMPILE = i686-elf-
		CCARCH = ia32
	else ifeq ($(ARCH),x86_64)
		CROSS_COMPILE = x86_64-elf-
		CCARCH = x86_64
	else
		$(error Unknown ARCH specified: $(ARCH))
	endif
else
	# Make sure CCARCH is defined
	CCARCH ?= $(shell $(CC) -dumpmachine | cut -f1 -d- | sed -e s,i[3456789]86,ia32, -e 's,armv7.*,arm,')
	ifeq ($(CCARCH),amd64)
		override CCARCH = x86_64
	endif
endif


# Define X86 as a shortcut for 'ia32 || x86_64'
ifneq (,$(filter $(ARCH),ia32 x86_64))
	X86 := 1
endif


###############################################################################
#
# Misc information
#
###############################################################################

GCCVERSION  := $(shell $(CC) -dumpversion | cut -f1 -d.)
GCCMINOR    := $(shell $(CC) -dumpversion | cut -f2 -d.)
GCCMACHINE  := $(shell $(CC) -dumpmachine)


###############################################################################
#
# Build flags
#
###############################################################################

ifeq ($(ARCH),ia32)
	ifeq ($(CCARCH),x86_64)
		ARCH_FLAGS += -m32
	endif
endif

ifeq ($(ARCH),x86_64)
	ifeq ($(CCARCH),ia32)
		ARCH_FLAGS += -m64
	endif
endif

CFLAGS += $(ARCH_FLAGS) -O2 -Wall -Wextra -Werror -ffreestanding -fbuiltin -fno-pic

CXXFLAGS += $(ARCH_FLAGS) -O2 -Wall -Wextra -Werror -ffreestanding -fbuiltin -fno-pic -std=gnu++20

ASFLAGS += $(ARCH_FLAGS) -fno-pic

LDFLAGS	+= -nostdlib --warn-common --no-undefined --fatal-warnings -z noexecstack

ifneq (mingw32,$(findstring mingw32, $(GCCMACHINE)))
LDFLAGS += -z max-page-size=0x1000
endif

DEFINES = __rainbow__

CPPFLAGS += $(addprefix -D,$(DEFINES)) $(addprefix -I,$(INCLUDES))

DEPFLAGS = -MMD -MP


###############################################################################
#
# Misc
#
###############################################################################

CRTBEGIN = $(shell $(CC) $(CFLAGS) -print-file-name=crtbegin.o)
CRTEND   = $(shell $(CC) $(CFLAGS) -print-file-name=crtend.o)
LIBC     = $(shell $(CC) $(CFLAGS) -print-file-name=libc.a)
LIBCXX   = $(shell $(CC) $(CFLAGS) -print-file-name=libstdc++.a)
LIBGCC   = $(shell $(CC) $(CFLAGS) -print-file-name=libgcc.a)


###############################################################################
#
# Detect CPU count
#
###############################################################################

ifeq ($(CPU_COUNT),)
	OS:=$(shell uname -s)
	ifeq ($(OS),Linux)
		CPU_COUNT := $(shell grep -c ^processor /proc/cpuinfo)
	else ifeq ($(OS),Darwin)
		CPU_COUNT := $(shell system_profiler | awk '/Number Of CPUs/{print $4}{next;}')
	else ifneq ($(NUMBER_OF_PROCESSORS),)
		CPU_COUNT := $(NUMBER_OF_PROCESSORS)
	else
		CPU_COUNT := 1
	endif
endif
