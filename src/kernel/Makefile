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

MODULES := . graphics metal

$(eval $(call load-modules,$(MODULES)))


###############################################################################
#
# Definitions
#
###############################################################################

INCLUDES += $(SRCDIR) $(TOPDIR) $(TOPDIR)/include $(TOPDIR)/../user/include $(TOPDIR)/third_party

DEFINES += ARCH=$(ARCH) __kernel__

OBJECTS = $(SOURCES:%=%.o)

DEPENDENCIES = $(OBJECTS:%.o=%.d)

CFLAGS += -Wno-address-of-packed-member

CXXFLAGS += -Wno-address-of-packed-member -Wno-invalid-offsetof

# TODO: we don't want to disable these warnings
CXXFLAGS += -Wno-array-bounds -Wno-stringop-overflow -Wno-stringop-overread

ARCH_FLAGS += -mgeneral-regs-only

ifeq ($(ARCH),ia32)
	ARCH_FLAGS += -mregparm=3
endif

ifeq ($(ARCH),x86_64)
	ARCH_FLAGS += -mno-red-zone -mcmodel=kernel
endif

ifdef X86
LDSCRIPT := $(SRCDIR)/x86/$(ARCH)/kernel.lds
else
LDSCRIPT := $(SRCDIR)/$(ARCH)/kernel.lds
endif

LDFLAGS += -T $(LDSCRIPT)

TARGETS := kernel


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


###############################################################################
#
# Rules
#
###############################################################################

CRTI = $(filter %/crti.S.o, $(OBJECTS))
CRTN = $(filter %/crtn.S.o, $(OBJECTS))
OBJECTS2 = $(filter-out $(CRTI) $(CRTN), $(OBJECTS))

kernel: $(OBJECTS) $(LDSCRIPT)
	$(LD) $(LDFLAGS) $(CRTI) $(CRTBEGIN) $(OBJECTS2) -lgcc $(CRTEND) $(CRTN) -o $@
	$(READELF) -a $@ > $@.elf
	$(OBJDUMP) -D $@ > $@.disassembly


-include $(DEPENDENCIES)
