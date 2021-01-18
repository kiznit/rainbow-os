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
TOPDIR := $(abspath $(SRCDIR)/../)/

vpath %.c $(SRCDIR) $(TOPDIR)
vpath %.S $(SRCDIR) $(TOPDIR)

include $(TOPDIR)/mk/defaults.mk
include $(TOPDIR)/mk/modules.mk
include $(TOPDIR)/mk/rules.mk


###############################################################################
#
# Definitions
#
###############################################################################

INCLUDES += $(SRCDIR)/src $(SRCDIR)/include $(TOPDIR)/include $(TOPDIR)/libposix/include $(TOPDIR) $(TOPDIR)/third_party

SOURCES := \
	src/thread.c \
	src/libc/malloc.c \
	src/libc/newlib.c \
	src/libc/arch/$(ARCH)/crt0.S \
	src/libc/arch/$(ARCH)/crti.S \
	src/libc/arch/$(ARCH)/crtn.S \


ifeq ($(ARCH),ia32)
	SOURCES += src/arch/ia32/syscall6.S
endif


DEFINES :=

OBJECTS = $(SOURCES:%=%.o)

DEPENDENCIES = $(OBJECTS:%.o=%.d)

TARGETS := librainbow.a crt0.o crti.o crtn.o malloc.o


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

# We do not link crt0/crti/crtn into the library, these are normally provided by libc.
CRT0 = $(filter %/crt0.S.o, $(OBJECTS))
CRTI = $(filter %/crti.S.o, $(OBJECTS))
CRTN = $(filter %/crtn.S.o, $(OBJECTS))

# malloc.o is a hack until we compile newlib with malloc support in a hosted compiler
MALLOC = $(filter %/malloc.c.o, $(OBJECTS))

OBJECTS_LIBRAINBOW = $(filter-out $(CRT0) $(CRTI) $(CRTN) $(MALLOC), $(OBJECTS))


librainbow.a: $(OBJECTS_LIBRAINBOW)
	$(AR) rcs $@ $^
	readelf -a $@ > $@.elf
	objdump -D $@ > $@.disassembly

crt0.o : $(CRT0)
	cp $< $@

crti.o : $(CRTI)
	cp $< $@

crtn.o : $(CRTN)
	cp $< $@

malloc.o : $(MALLOC)
	cp $< $@


-include $(DEPENDENCIES)
