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
TOPDIR := $(abspath $(SRCDIR)/../..)/

vpath %.c $(SRCDIR) $(TOPDIR)
vpath %.cpp $(SRCDIR) $(TOPDIR)
vpath %.S $(SRCDIR) $(TOPDIR)

include $(TOPDIR)/mk/defaults.mk
include $(TOPDIR)/mk/modules.mk
include $(TOPDIR)/mk/rules.mk


###############################################################################
#
# Modules
#
###############################################################################

MODULEPATH := $(SRCDIR) $(TOPDIR)

MODULES := .

$(eval $(call load-modules,$(MODULES)))


###############################################################################
#
# Definitions
#
###############################################################################

INCLUDES += $(SRCDIR) $(TOPDIR)/include $(TOPDIR)/libc/include

DEFINES +=

OBJECTS = $(SOURCES:%=%.o)

DEPENDENCIES = $(OBJECTS:%.o=%.d)

LDSCRIPT := $(TOPDIR)/libc/linker/$(ARCH).lds

LDFLAGS += -T $(LDSCRIPT) -L../../libc

TARGETS := go


###############################################################################
#
# Targets
#
###############################################################################

.PHONY: all
all: $(TARGETS)

.PHONY: clean
clean:
	$(RM) $(OBJECTS) $(DEPENDENCIES) $(TARGETS)

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

go: $(OBJECTS) $(LDSCRIPT)
	$(LD) $(LDFLAGS) $(OBJECTS) -lc $(LIBGCC) -o $@
	readelf -a $@ > $@.elf
	objdump -D $@ > $@.disassembly


-include $(DEPENDENCIES)
