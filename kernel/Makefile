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

MKFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
SRCDIR := $(dir $(MKFILE_PATH))
TOPDIR := $(SRCDIR)/..
VPATH := $(SRCDIR) $(TOPDIR)

include $(TOPDIR)/defaults.mk


###############################################################################
#
# Definitions
#
###############################################################################

SOURCES := \
	console.cpp \
	kernel.cpp \
	pmm.cpp \
	graphics/graphicsconsole.cpp \
	graphics/pixels.cpp \
	graphics/vgafont.cpp \
	metal/crt.cpp \
	metal/log.cpp

INCLUDES += $(SRCDIR) $(TOPDIR)

DEFINES += ARCH=$(ARCH)

LDSCRIPT := $(SRCDIR)/$(ARCH)/kernel.lds

TARGETS := kernel

ifeq ($(ARCH),x86_64)
	ARCH_FLAGS += -mno-red-zone -mcmodel=kernel -mno-mmx -mno-sse
endif


# Include arch-specific source code
ifneq (,$(filter $(ARCH),ia32 x86_64))
	include $(SRCDIR)/x86/arch.mk
else
	include $(SRCDIR)/$(ARCH)/arch.mk
endif


LDFLAGS += -T $(LDSCRIPT)

OBJECTS := $(SOURCES:%=%.o)

DEPENDENCIES := $(OBJECTS:%.o=%.d)


###############################################################################
#
# Targets
#
###############################################################################

.PHONY: all
all: $(TARGETS)


.PHONY: mkvars
mkvars:
	@echo AR=$(AR)
	@echo ARCH=$(ARCH)
	@echo AS=$(AS)
	@echo CC=$(CC)
	@echo GCCVERSION=$(GCCVERSION)
	@echo GCCMINOR=$(GCCMINOR)
	@echo HOSTARCH=$(HOSTARCH)
	@echo LD=$(LD)
	@echo OBJCOPY=$(OBJCOPY)
	@echo OS=$(OS)
	@echo prefix=$(prefix)
	@echo RANLIB=$(RANLIB)
	@echo SRCDIR=$(SRCDIR)
	@echo TOPDIR=$(TOPDIR)
	@echo SOURCES=$(SOURCES)
	@echo OBJECTS=$(OBJECTS)
	@echo TARGETS=$(TARGETS)


###############################################################################
#
# Rules
#
###############################################################################

kernel: $(OBJECTS) $(LDSCRIPT)
	$(LD) $(LDFLAGS) $(OBJECTS) $(LIBGCC) -o $@


include $(TOPDIR)/rules.mk

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPENDENCIES)
endif
