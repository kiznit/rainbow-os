# Copyright (c) 2018, Thierry Tremblay
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

HOSTCC      := $(prefix)gcc
CC          := $(prefix)$(CROSS_COMPILE)gcc
AS          := $(prefix)$(CROSS_COMPILE)as
LD          := $(prefix)$(CROSS_COMPILE)ld
AR          := $(prefix)$(CROSS_COMPILE)ar
RANLIB      := $(prefix)$(CROSS_COMPILE)ranlib
OBJCOPY     := $(prefix)$(CROSS_COMPILE)objcopy


###############################################################################
#
# Host / target
#
###############################################################################

# Arch of the host machine
HOSTARCH    ?= $(shell $(HOSTCC) -dumpmachine | cut -f1 -d- | sed -e s,i[3456789]86,ia32, -e 's,armv7.*,arm,' )

# Default arch of the compiler
CCARCH      ?= $(shell $(CC) -dumpmachine | cut -f1 -d- | sed -e s,i[3456789]86,ia32, -e 's,armv7.*,arm,' )

# User specified arch
ARCH        ?= $(shell $(CC) -dumpmachine | cut -f1 -d- | sed -e s,i[3456789]86,ia32, -e 's,armv7.*,arm,' )

# FreeBSD (and possibly others) reports amd64 instead of x86_64
ifeq ($(HOSTARCH),amd64)
	override HOSTARCH := x86_64
endif

ifeq ($(CCARCH),amd64)
	override CCARCH := x86_64
endif

ifeq ($(ARCH),amd64)
	override ARCH := x86_64
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
	ARCH_FLAGS += -mno-mmx -mno-sse
endif

ifeq ($(ARCH),x86_64)
	ifeq ($(CCARCH),ia32)
		ARCH_FLAGS += -m64
	endif
	ARCH_FLAGS += -mno-mmx -mno-sse
endif

ifdef X86
	# Disable AVX, if the compiler supports that.
	CC_CAN_DISABLE_AVX=$(shell $(CC) -Werror -c -o /dev/null -xc -mno-avx - </dev/null >/dev/null 2>&1 && echo 1)
	ifeq ($(CC_CAN_DISABLE_AVX), 1)
		ARCH_FLAGS += -mno-avx
	endif
endif


CFLAGS += $(ARCH_FLAGS) -O2 -Wall -Wextra -Werror -ffreestanding -fbuiltin -fno-pic

CXXFLAGS += $(ARCH_FLAGS) -O2 -Wall -Wextra -Werror -ffreestanding -fbuiltin -fno-pic -fno-exceptions -fno-rtti

ASFLAGS += $(ARCH_FLAGS) -fno-pic

LDFLAGS	+= -nostdlib --warn-common --no-undefined --fatal-warnings

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

LIBGCC      := $(shell $(CC) $(CFLAGS) -print-file-name=libgcc.a)
