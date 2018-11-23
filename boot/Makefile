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
VPATH := $(SRCDIR) $(TOPDIR)/shared $(TOPDIR)/third_party

include $(TOPDIR)/defaults.mk


###############################################################################
#
# Definitions
#
###############################################################################

GNUEFIDIR := $(TOPDIR)/third_party/gnuefi

SOURCES := \
	log.cpp \
	efi/efiboot.cpp \
	efi/eficonsole.cpp \
	gnuefi/gnuefi/crt0-efi-$(ARCH).S \
	gnuefi/gnuefi/reloc_$(ARCH).c \
	gnuefi/lib/boxdraw.c \
	gnuefi/lib/smbios.c \
	gnuefi/lib/console.c \
	gnuefi/lib/crc.c \
	gnuefi/lib/data.c \
	gnuefi/lib/debug.c \
	gnuefi/lib/dpath.c \
	gnuefi/lib/error.c \
	gnuefi/lib/event.c \
	gnuefi/lib/exit.c \
	gnuefi/lib/guid.c \
	gnuefi/lib/hand.c \
	gnuefi/lib/hw.c \
	gnuefi/lib/init.c \
	gnuefi/lib/lock.c \
	gnuefi/lib/misc.c \
	gnuefi/lib/print.c \
	gnuefi/lib/sread.c \
	gnuefi/lib/str.c \
	gnuefi/lib/cmdline.c \
	gnuefi/lib/runtime/rtlock.c \
	gnuefi/lib/runtime/efirtlib.c \
	gnuefi/lib/runtime/rtstr.c \
	gnuefi/lib/runtime/vm.c \
	gnuefi/lib/runtime/rtdata.c \
	gnuefi/lib/$(ARCH)/initplat.c \
	gnuefi/lib/$(ARCH)/math.c \
	gnuefi/lib/$(ARCH)/setjmp.S \
	graphics/graphicsconsole.cpp \
	graphics/pixels.cpp \
	graphics/vgafont.cpp


INCLUDES += $(SRCDIR) $(TOPDIR)/shared $(GNUEFIDIR)/inc $(GNUEFIDIR)/inc/$(ARCH) $(GNUEFIDIR)/inc/protocol $(GNUEFIDIR)/lib

DEFINES += BOOT_ARCH=$(ARCH)

OBJECTS := $(SOURCES:%=%.o)

DEPENDENCIES := $(OBJECTS:%.o=%.d)

LDSCRIPT := $(GNUEFIDIR)/gnuefi/elf_$(ARCH)_efi.lds

TARGETS := boot.efi


CFLAGS += --std=gnu11 -fshort-wchar -fno-strict-aliasing
CXXFLAGS += --std=gnu++11 -fshort-wchar -fno-strict-aliasing

ifeq ($(ARCH),ia32)
	CXXFLAGS += -D__STDC_VERSION__=201112 # Required to prevent conflicting int32_t and uint32_t definitions in gnuefi
else ifeq ($(ARCH),x86_64)
	ARCH_FLAGS += -mno-red-zone
	DEFINES += GNU_EFI_USE_MS_ABI
endif

# Only enable -fpic for non MinGW compilers (unneeded on MinGW)
GCCMACHINE := $(shell $(CC) -dumpmachine)
ifneq (mingw32,$(findstring mingw32, $(GCCMACHINE)))
	CFLAGS += -fpic
	CXXFLAGS += -fpic
endif

LDFLAGS += -shared -Bsymbolic -T $(LDSCRIPT)

LIBGCC := $(shell $(CC) $(CFLAGS) -print-file-name=libgcc.a)


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

%.efi: %.so
	$(OBJCOPY) -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel -j .rela -j .rel.* -j .rela.* -j .rel* -j .rela* -j .reloc --target efi-app-$(ARCH) $*.so $@

%.so: $(OBJECTS)
	$(LD) $(LDFLAGS) $^ $(LIBGCC) -o $@


include $(TOPDIR)/rules.mk


ifneq ($(MAKECMDGOALS),clean)
-include $(DEPENDENCIES)
endif
