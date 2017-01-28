# Copyright (c) 2017, Thierry Tremblay
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

# Make sure we are using bash
SHELL := /bin/bash



###############################################################################
#
# Source
#
###############################################################################

SOURCES := $(TARGET_ARCH)/entry.s kernel.cpp

OBJECTS := $(SOURCES:%=$(BUILDDIR)/%.o)

DEPENDENCIES := $(OBJECTS:%.o=%.d)



###############################################################################
#
# Toolchain
#
###############################################################################

include $(SRCDIR)/mk/toolchain.mk

ifeq ($(TARGET_ARCH),ia32)
	ARCH_FLAGS := $(ARCH_FLAGS) -mno-mmx -mno-sse
else ifeq ($(TARGET_ARCH),x86_64)
	ARCH_FLAGS := $(ARCH_FLAGS) -mcmodel=kernel -mno-red-zone -mno-mmx -mno-sse
endif

CFLAGS  := $(CFLAGS) -ffreestanding -fbuiltin
CXXLAGS := $(CXXFLAGS) -ffreestanding -fbuiltin -fno-exceptions -fno-rtti
ASFLAGS := $(ASFLAGS) -ffreestanding -fbuiltin
LDFLAGS := $(LDFLAGS) -nostdlib -T $(TARGET_ARCH)/kernel.ld

ifeq ($(TARGET_ARCH),x86_64)
	LDFLAGS := $(LDFLAGS) -z max-page-size=0x1000
endif



###############################################################################
#
# Targets
#
###############################################################################

.PHONY: all
all: $(BUILDDIR)/kernel

$(BUILDDIR)/kernel: $(OBJECTS) $(TARGET_ARCH)/kernel.ld
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) $(OBJECTS) -o $@



.PHONY: clean
clean:
	@test -n "$(BUILDDIR)" && rm -rf $(OBJECTS) $(DEPENDENCIES) $(BUILDDIR)/kernel



###############################################################################
#
# Rules
#
###############################################################################

include $(SRCDIR)/mk/rules.mk


ifneq ($(MAKECMDGOALS),clean)
-include $(DEPENDENCIES)
endif
