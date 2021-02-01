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

CRT0       := ../../libs/rainbow/crt0.o
CRTI       := ../../libs/rainbow/crti.o
CRTN       := ../../libs/rainbow/crtn.o

LIBC       := ../../libs/rainbow/malloc.o $(LIBC)
LIBPTHREAD := --whole-archive -lpthread --no-whole-archive

LDSCRIPT   := $(TOPDIR)/libs/rainbow/src/runtime/arch/$(ARCH)/user.lds

LDFLAGS = -T $(LDSCRIPT) $(addprefix -L../../libs/,$(LIBS))


LINK_DEPS  = $(OBJECTS) $(LDSCRIPT) $(CRT0) $(CRTI) $(CRTO) $(foreach LIB,$(LIBS),../../libs/$(LIB)/lib$(LIB).a)