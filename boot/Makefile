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

MKFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
SRCDIR := $(dir $(MKFILE_PATH))
TOPDIR := $(abspath $(SRCDIR)/..)/

vpath %.cpp $(SRCDIR) $(TOPDIR)
vpath %.S $(SRCDIR) $(TOPDIR)

include $(TOPDIR)/mk/defaults.mk
include $(TOPDIR)/mk/modules.mk
include $(TOPDIR)/mk/rules.mk

# Save arch for the kernel
KERNEL_ARCH := $(ARCH)

ifeq ($(MACHINE),bios)

	# When I use my x86_64 bits cross compiler, it can't find libgcc for ia32.
	# This is because it doesn't support multilib, which I can't seem to figure out.
	# So the workaround is to use my ia32 cross compiler (if it can be found).
	ifeq ($(CROSS_COMPILE),x86_64-elf-)
		ARCH = ia32
		override CROSS_COMPILE = i686-elf-
	endif
endif


###############################################################################
#
# Modules
#
###############################################################################

MODULEPATH := $(SRCDIR) $(TOPDIR)

MODULES := . graphics metal

$(eval $(call load-modules,$(MODULES)))


###############################################################################
#
# Definitions
#
###############################################################################

INCLUDES += $(SRCDIR) $(TOPDIR) $(TOPDIR)/include $(TOPDIR)/third_party

ifeq ($(MACHINE),efi)
INCLUDES += $(TOPDIR)/third_party/uefi
endif

DEFINES += KERNEL_ARCH=$(KERNEL_ARCH)

ifeq ($(KERNEL_ARCH),ia32)
	DEFINES += KERNEL_IA32
else ifeq ($(KERNEL_ARCH),x86_64)
	DEFINES += KERNEL_X86_64
endif

OBJECTS = $(SOURCES:%=%.o)

DEPENDENCIES = $(OBJECTS:%.o=%.d)

ARCH_FLAGS += -mgeneral-regs-only

ifneq ($(LDSCRIPT),)
	LDFLAGS += -T $(LDSCRIPT)
endif

TARGETS ?= bootloader


###############################################################################
#
# Targets
#
###############################################################################

.PHONY: all
all: $(TARGETS)

.PHONY: clean
clean:
	$(RM) $(OBJECTS) $(DEPENDENCIES) $(TARGETS) boot.so

.PHONY: mkvars
mkvars:
	@echo AR=$(AR)
	@echo ARCH=$(ARCH)
	@echo AS=$(AS)
	@echo CC=$(CC)
	@echo CCARCH=$(CCARCH)
	@echo GCCVERSION=$(GCCVERSION)
	@echo GCCMINOR=$(GCCMINOR)
	@echo HOSTARCH=$(HOSTARCH)
	@echo KERNEL_ARCH=$(KERNEL_ARCH)
	@echo LD=$(LD)
	@echo LIBGCC=$(LIBGCC)
	@echo MACHINE=$(MACHINE)
	@echo OBJCOPY=$(OBJCOPY)
	@echo prefix=$(prefix)
	@echo RANLIB=$(RANLIB)
	@echo SRCDIR=$(SRCDIR)
	@echo TOPDIR=$(TOPDIR)
	@echo MODULEPATH=$(MODULEPATH)
	@echo SOURCES=$(SOURCES)
	@echo OBJECTS=$(OBJECTS)
	@echo LDSCRIPT=$(LDSCRIPT)
	@echo TARGETS=$(TARGETS)


###############################################################################
#
# Rules
#
###############################################################################

bootloader: $(OBJECTS) $(LDSCRIPT)
	$(LD) $(LDFLAGS) $(OBJECTS) $(LIBC) $(LIBGCC) -o $@
	readelf -a $@ > $@.elf
	objdump -D $@ > $@.disassembly


-include $(DEPENDENCIES)
