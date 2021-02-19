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

# TODO: this whole file is temporary until we have a hosted toolchain

vpath %.c $(SRCDIR) $(TOPDIR)
vpath %.cpp $(SRCDIR) $(TOPDIR)
vpath %.S $(SRCDIR) $(TOPDIR)

include $(TOPDIR)/../src/mk/defaults.mk
include $(TOPDIR)/../src/mk/rules.mk

CRT0       := ../../libs/libc/crt0.o
CRTI       := ../../libs/libc/crti.o
CRTN       := ../../libs/libc/crtn.o

# Linking with --whole-archive is required to support static libc + libgcc. The issue is that libgcc
# defines weak symbols to the pthread functions and we get a successful link but a crashing app when
# throwing C++ exceptions. The end goal is to use libc.so, but we are not there yet.
LIBC       := --whole-archive ../../libs/libc/libc.a --no-whole-archive $(LIBC)

LDSCRIPT   := $(TOPDIR)/libs/libc/src/runtime/$(ARCH)/user.lds

CFLAGS = $(ARCH_FLAGS) -O2 -Wall -Wextra -Werror -std=gnu17

CXXFLAGS = $(ARCH_FLAGS) -O2 -Wall -Wextra -Werror -std=gnu++20

ASFLAGS = $(ARCH_FLAGS)

LDFLAGS = -T $(LDSCRIPT) $(addprefix -L../../libs/,$(LIBS))

ifeq ($(ARCH),x86_64)
# The linker wants to use 2MB pages by default, we don't like that
LDFLAGS += -z max-page-size=0x1000
endif

LINK_DEPS  = $(OBJECTS) $(LDSCRIPT) $(CRT0) $(CRTI) $(CRTO) $(foreach LIB,$(LIBS),../../libs/$(LIB)/lib$(LIB).a)
