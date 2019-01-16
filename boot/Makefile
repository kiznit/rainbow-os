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
TOPDIR := $(abspath $(SRCDIR)/..)/
VPATH := $(SRCDIR) $(TOPDIR)

include $(TOPDIR)/defaults.mk
include $(TOPDIR)/rules.mk


###############################################################################
#
# Modules
#
###############################################################################

MODULEPATH := $(SRCDIR) $(TOPDIR)

MODULES := . efi graphics metal

$(eval $(call load-modules,$(MODULES)))


###############################################################################
#
# Definitions
#
###############################################################################

INCLUDES += $(SRCDIR) $(TOPDIR) $(TOPDIR)/third_party $(TOPDIR)/third_party/uefi

DEFINES += BOOT_ARCH=$(ARCH)

OBJECTS = $(SOURCES:%=%.o)

DEPENDENCIES = $(OBJECTS:%.o=%.d)

LDSCRIPT := $(SRCDIR)/efi/$(ARCH)/efi.lds

TARGETS := boot.efi


CFLAGS += -fpic -fshort-wchar
CXXFLAGS += -fpic -fshort-wchar
LDFLAGS += -shared -Bsymbolic -T $(LDSCRIPT)

ifeq ($(ARCH),x86_64)
	ARCH_FLAGS += -mno-red-zone
endif



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

boot.so: $(OBJECTS) $(LDSCRIPT)
	$(LD) $(LDFLAGS) $(OBJECTS) $(LIBGCC) -o $@

%.efi: %.so
	$(OBJCOPY) -j .text -j .rodata -j .data -j .dynamic -j .dynsym -j .rel.* -j .rela.* -j .reloc -j .init_array --target efi-app-$(ARCH) $*.so $@


-include $(DEPENDENCIES)
