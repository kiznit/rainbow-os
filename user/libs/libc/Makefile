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

include $(TOPDIR)/mk/user.mk


###############################################################################
#
# Definitions
#
###############################################################################

INCLUDES += $(SRCDIR)/src $(TOPDIR)/include $(TOPDIR)/../src/third_party

SOURCES := \
	src/internal/newlib.c \
	src/internal/tls.c \
	src/runtime/crt0.c \
	src/runtime/$(ARCH)/crti.S \
	src/runtime/$(ARCH)/crtn.S \
	src/stdlib/malloc.c \
	src/thread/pthread_cond.c \
	src/thread/pthread_key.c \
	src/thread/pthread_mutex.c \
	src/thread/pthread_once.c \
	src/thread/pthread_rwlock.c \
	src/thread/pthread_thread.c

ifeq ($(ARCH),ia32)
	SOURCES += src/internal/ia32/syscall6.S
endif

DEFINES +=

OBJECTS = $(SOURCES:%=%.o)

DEPENDENCIES = $(OBJECTS:%.o=%.d)

TARGETS = libc.a $(CRT0_TARGET) $(CRTI_TARGET) $(CRTN_TARGET)


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
CRT0 = $(filter %/crt0.c.o, $(OBJECTS))
CRTI = $(filter %/crti.S.o, $(OBJECTS))
CRTN = $(filter %/crtn.S.o, $(OBJECTS))

OBJECTS_LIBC = $(filter-out $(CRT0) $(CRTI) $(CRTN), $(OBJECTS))

libc.a: $(OBJECTS_LIBC)
	$(AR) rcs $@ $^
	readelf -a $@ > $@.elf
	objdump -D $@ > $@.disassembly

$(CRT0_TARGET) : $(CRT0)
	cp $< $@

$(CRTI_TARGET) : $(CRTI)
	cp $< $@

$(CRTN_TARGET) : $(CRTN)
	cp $< $@

-include $(DEPENDENCIES)
