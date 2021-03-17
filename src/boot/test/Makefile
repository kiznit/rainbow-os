# Copyright (c) 2021, Thierry Tremblay
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
SRCDIR := $(dir $(MKFILE_PATH))/..
TOPDIR := $(abspath $(SRCDIR)/..)/

vpath %.c $(SRCDIR) $(TOPDIR)
vpath %.cpp $(SRCDIR) $(TOPDIR)
vpath %.S $(SRCDIR) $(TOPDIR)


###############################################################################
#
# Toolchain
#
###############################################################################

CC ?= gcc

CFLAGS   += -Wall -Wextra -Werror -std=gnu17 -fprofile-arcs -ftest-coverage
CXXFLAGS += -Wall -Wextra -Werror -std=gnu++20 -fprofile-arcs -ftest-coverage
ASFLAGS  += -Wall -Wextra -Werror -fprofile-arcs -ftest-coverage
LDFLAGS  += -Wl,--warn-common -Wl,--no-undefined -Wl,--fatal-warnings -z noexecstack -lgcov --coverage

CPPFLAGS += $(addprefix -D,$(DEFINES)) $(addprefix -I,$(INCLUDES))

DEPFLAGS = -MMD -MP


###############################################################################
#
# Definitions
#
###############################################################################

SOURCES = \
	boot.cpp \
	display.cpp \
	elfloader.cpp \
	memory.cpp \
	memory.test.cpp \
	arch/x86/check.cpp \
	arch/x86/vmm.cpp \
	test/mock.cpp \
	test/test.cpp \
	graphics/edid.cpp \
	graphics/graphicsconsole.cpp \
	graphics/pixels.cpp \
	graphics/vgafont.cpp

INCLUDES = \
	$(SRCDIR) \
	$(TOPDIR) \
	$(TOPDIR)/include \
	$(TOPDIR)/metal/include \
	$(TOPDIR)/third_party

DEFINES = \
	KERNEL_ARCH=x86_64 \
	KERNEL_X86_64

OBJECTS = $(SOURCES:%=%.o)

DEPENDENCIES = $(OBJECTS:%.o=%.d)

TARGET = boot_tests


###############################################################################
#
# TARGET
#
###############################################################################

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	$(RM) $(OBJECTS) $(DEPENDENCIES) $(TARGET)

.PHONY: test
test: $(TARGET)
	./$(TARGET)


###############################################################################
#
# Rules
#
###############################################################################

%.c.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(DEPFLAGS) -c $< -o $(@:%.d=%.o)

%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CXXFLAGS) $(CPPFLAGS) $(DEPFLAGS) -c $< -o $(@:%.d=%.o)

%.S.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) $(CPPFLAGS) $(DEPFLAGS) -c $< -o $(@:%.d=%.o)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -lstdc++ -lm -o $@


-include $(DEPENDENCIES)
