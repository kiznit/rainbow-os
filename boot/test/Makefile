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

CC	:= gcc
CXX	:= g++
LD	:= ld
AS	:= as

BUILDDIR ?= $(CURDIR)

googletest_root := $(SRCDIR)/third_party/googletest/googletest
DEFINES +=
INCLUDES += $(SRCDIR)/boot $(SRCDIR)/headers $(googletest_root)/include $(googletest_root)/include
SOURCES := main.cpp test_arm.cpp test_memory.cpp ../memory.cpp ../raspi/arm.cpp $(googletest_root)/src/gtest-all.cc
OBJECTS := $(notdir $(SOURCES:%=%.o))
OBJECTS := $(OBJECTS:%=$(BUILDDIR)/%)
DEPENDENCIES := $(OBJECTS:%.o=%.d)

CPPFLAGS	+= $(addprefix -D,$(DEFINES)) $(addprefix -I,$(INCLUDES))
CFLAGS		+= -O2 -Wall -Wextra -Werror -std=gnu99
CXXFLAGS	+= -O2 -Wall -Wextra -Werror -std=gnu++11
ASFLAGS		+= -O2 -Wall -Wextra -Werror
LDFLAGS		+= -pthread



.PHONY: all
all: $(BUILDDIR)/test

$(BUILDDIR)/test: $(OBJECTS)
	$(CXX) $(LDFLAGS) $(OBJECTS) -o $@


.PHONY: clean
clean:
	$(RM) $(OBJECTS) $(DEPENDENCIES) $(BUILDDIR)/test
	@-rmdir $(BUILDDIR)



.SUFFIXES:
.SUFFIXES: .c .cpp .h. .hpp .s .o

$(BUILDDIR)/%.c.o $(BUILDDIR)/%.c.d: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -MMD -c $< -o $(@:%.d=%.o)

$(BUILDDIR)/%.cpp.o $(BUILDDIR)/%.cpp.d: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -MMD -c $< -o $(@:%.d=%.o)

$(BUILDDIR)/%.s.o $(BUILDDIR)/%.s.d: %.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $(CPPFLAGS) -Wa,--MD,$@ -c $< -o $(@:%.d=%.o)

$(BUILDDIR)/%.cpp.o $(BUILDDIR)/%.cpp.d: ../%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -MMD -c $< -o $(@:%.d=%.o)

$(BUILDDIR)/%.cpp.o $(BUILDDIR)/%.cpp.d: ../raspi/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -MMD -c $< -o $(@:%.d=%.o)

$(BUILDDIR)/gtest-all.cc.o $(BUILDDIR)/gtest-all.cc.d: $(googletest_root)/src/gtest-all.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -I$(googletest_root) -MMD -c $< -o $(@:%.d=%.o)



ifneq ($(MAKECMDGOALS),clean)
-include $(DEPENDENCIES)
endif
