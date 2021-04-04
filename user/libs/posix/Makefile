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

INCLUDES += $(SRCDIR)/src $(SRCDIR)/include $(TOPDIR)/include

SOURCES := \
	src/_exit.c \
	src/close.c \
	src/fstat.c \
	src/getpid.c \
	src/gettimeofday.c \
	src/isatty.c \
	src/kill.c \
	src/lseek.c \
	src/mmap.c \
	src/munmap.c \
	src/read.c \
	src/sched_yield.c \
	src/sleep.c \
	src/sysconf.c \
	src/usleep.c \
	src/write.c

DEFINES +=

OBJECTS = $(SOURCES:%=%.o)

DEPENDENCIES = $(OBJECTS:%.o=%.d)

TARGETS := libposix.a


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

.PHONY: install
install:
	cp libposix.a $(LIBC_DIR)/libposix.a


###############################################################################
#
# Rules
#
###############################################################################

libposix.a: $(OBJECTS)
	$(AR) rcs $@ $^
	$(READELF) -a $@ > $@.elf
	$(OBJDUMP) -D $@ > $@.disassembly


-include $(DEPENDENCIES)
