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
# Toolchain
#
###############################################################################

HOSTCC      = $(prefix)gcc
AR          = $(prefix)$(CROSS_COMPILE)ar
CC          = $(prefix)$(CROSS_COMPILE)gcc
AS          = $(prefix)$(CROSS_COMPILE)gcc
LD          = $(prefix)$(CROSS_COMPILE)gcc
OBJCOPY     = $(prefix)$(CROSS_COMPILE)objcopy
OBJDUMP     = $(prefix)$(CROSS_COMPILE)objdump
RANLIB      = $(prefix)$(CROSS_COMPILE)ranlib
READELF     = $(prefix)$(CROSS_COMPILE)readelf
STRIP       = $(prefix)$(CROSS_COMPILE)strip


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
		# No arch or cross compiler specified, try to guess from machine
		ifneq (,$(filter $(MACHINE),raspi raspi2))
			ARCH = arm
		else ifneq (,$(filter $(MACHINE),raspi3 raspi4))
			ARCH = aarch64
		else
			# Fallback: use host arch
			ARCH = $(HOSTARCH)
		endif
	endif
else ifeq ($(ARCH),amd64)
	override ARCH = x86_64
endif

# Select the right cross comnpiler
ifeq ($(CROSS_COMPILE),)
	ifeq ($(ARCH),ia32)
		CROSS_COMPILE = i686-rainbow-elf-
		CCARCH = ia32
	else ifeq ($(ARCH),x86_64)
		CROSS_COMPILE = x86_64-rainbow-elf-
		CCARCH = x86_64
	else ifeq ($(ARCH),arm)
		CROSS_COMPILE = arm-rainbow-eabi-
		CCARCH = arm
	else ifeq ($(ARCH),aarch64)
		CROSS_COMPILE = aarch64-rainbow-elf-
		CCARCH = aarch64
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
else ifeq ($(ARCH),x86_64)
	ifeq ($(CCARCH),ia32)
		ARCH_FLAGS += -m64
	endif
else ifeq ($(ARCH),arm)
	ifeq ($(MACHINE),raspi)
		# BCM2835
		ARCH_FLAGS ?= mfloat-abi=hard -march=armv6kz -mtune=arm1176jzf-s -mfpu=vfp
	else ifeq ($(MACHINE),raspi2)
		# BCM2836
		ARCH_FLAGS ?= -mfloat-abi=hard -march=armv7-a -mtune=cortex-a7 -mfpu=neon-vfpv4
	else ifeq ($(MACHINE),raspi3)
		# BCM2837
		ARCH_FLAGS ?= -mfloat-abi=hard -march=armv8-a+crc -mtune=cortex-a53 -mfpu=crypto-neon-fp-armv8
	endif
else ifeq ($(ARCH),aarch64)
	ifeq ($(MACHINE),raspi3)
		ARCH_FLAGS ?= -march=armv8-a+crc -mtune=cortex-a53
	else ifeq ($(MACHINE),raspi4)
		ARCH_FLAGS ?= -march=armv8-a+crc -mtune=cortex-a72
	endif
endif



CFLAGS += $(ARCH_FLAGS) -fno-pic -O3 -Wall -Wextra -Werror -ffreestanding -fbuiltin -fno-strict-aliasing -fwrapv -std=gnu2x

CXXFLAGS += $(ARCH_FLAGS) -fno-pic -O3 -Wall -Wextra -Werror -ffreestanding -fbuiltin -fno-strict-aliasing -fwrapv -std=gnu++20 -fno-exceptions -fno-rtti

ASFLAGS += $(ARCH_FLAGS) -fno-pic -Wall -Wextra -Werror

LDFLAGS += $(CXXFLAGS) -nostdlib -Wl,--warn-common -Wl,--no-undefined -Wl,--fatal-warnings -z noexecstack

ifneq (mingw32,$(findstring mingw32, $(GCCMACHINE)))
LDFLAGS += -z max-page-size=0x1000
endif

CPPFLAGS += $(addprefix -D,$(DEFINES)) $(addprefix -I,$(INCLUDES))

DEPFLAGS = -MMD -MP


###############################################################################
#
# Misc
#
###############################################################################

CRTBEGIN = $(shell $(CC) $(CXXFLAGS) -print-file-name=crtbegin.o)
CRTEND   = $(shell $(CC) $(CXXFLAGS) -print-file-name=crtend.o)


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
