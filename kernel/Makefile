# Copyright (c) 2015, Thierry Tremblay
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

TARGET_ARCH ?= x86_64

include ../Makefile.common

GLOBAL_INCLUDES := ../include

MODULES := .

$(eval $(call load-modules,$(MODULES)))


ifeq ($(TARGET_ARCH),x86_64)
CFLAGS		:= $(CFLAGS) -mcmodel=kernel -mno-red-zone
CXXFLAGS	:= $(CXXFLAGS) -mcmodel=kernel -mno-red-zone
endif

LDFLAGS		:= $(LDFLAGS) -T $(TARGET_ARCH)/kernel.ld



.PHONY all:
all: $(BUILDDIR)/bin/kernel

$(BUILDDIR)/bin/kernel: $(OBJECTS) $(TARGET_ARCH)/kernel.ld
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) $(OBJECTS) -o $@


.PHONY: clean
clean:
	$(RM) -r $(BUILDDIR)


-include $(DEPENDENCIES)
