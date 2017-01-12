# Copyright (c) 2016, Thierry Tremblay
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

SHELL = bash

BUILDDIR ?= build



###############################################################################
#
# Detect host OS
#
###############################################################################

ifndef HOST_OS
	ifeq ($(OS),Windows_NT)
		HOST_OS := Windows
	else
		# Linux, FreeBSD, Darwin, SunOS, ...
		HOST_OS := $(shell uname -s)
	endif
endif

ifndef HOST_OS
$(error HOST_OS not detected)
endif



###############################################################################
#
# Detect host architecture
#
###############################################################################

ifndef HOST_ARCH
	ifeq ($(HOST_OS),Windows)
		ifeq ($(PROCESSOR_ARCHITEW6432),AMD64)
			HOST_ARCH := x86_64
		else ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
			HOST_ARCH := x86_64
		else
			HOST_ARCH := ia32
		endif
	else
		# -m is the only option supported everywhere, see https://en.wikipedia.org/wiki/Uname
		machine := $(shell uname -m)

		ifneq ($(filter $(machine),x86_64 amd64 i686-64),)
			HOST_ARCH := x86_64
		else ifneq ($(filter $(machine),i386 i686 i86pc x86pc i686-AT386),)
			HOST_ARCH := ia32
		endif
	endif
endif


ifndef HOST_ARCH
$(error HOST_ARCH not detected)
endif


$(info HOST_ARCH=$(HOST_ARCH))



###############################################################################
#
# Determine target architecture
#
###############################################################################

TARGET_ARCH ?= $(HOST_ARCH)


$(info TARGET_ARCH=$(TARGET_ARCH))



###############################################################################
#
# Toolchain
#
###############################################################################

ifndef TOOLPREFIX
	ifeq ($(TARGET_ARCH),ia32)
		TOOLPREFIX := i686-elf-
	else ifeq ($(TARGET_ARCH),x86_64)
		TOOLPREFIX := x86_64-elf-
	else ifeq ($(TARGET_ARCH),arm)
		TOOLPREFIX := arm-none-eabi-
	else ifeq ($(TARGET_ARCH),aarch64)
		TOOLPREFIX := aarch64-none-elf-
	endif
endif

CC	:= $(TOOLPREFIX)gcc
CXX	:= $(TOOLPREFIX)gcc
AS	:= $(TOOLPREFIX)gcc
LD	:= $(TOOLPREFIX)ld



###############################################################################
#
# Flags
#
###############################################################################

ifeq ($(TARGET_ARCH),ia32)
	ARCH_FLAGS ?= -march=i686 -msoft-float -mno-mmx -mno-sse

else ifeq ($(TARGET_ARCH),x86_64)
	ARCH_FLAGS ?= -mcmodel=kernel -mno-red-zone -msoft-float -mno-mmx -mno-sse
endif


CFLAGS		:= $(ARCH_FLAGS) -O2 -ffreestanding -fbuiltin -Wall -Wextra -Werror -std=gnu99
CXXFLAGS	:= $(ARCH_FLAGS) -O2 -ffreestanding -fbuiltin -Wall -Wextra -Werror -std=gnu++11 -fno-exceptions -fno-rtti
ASFLAGS		:= $(ARCH_FLAGS) -O2 -ffreestanding -fbuiltin -Wall -Wextra -Werror
LDFLAGS		:= -nostdlib --warn-common --no-undefined --fatal-warnings

DEFINES 	:=
INCLUDES 	:= include

CPPFLAGS	= $(addprefix -D,$(DEFINES)) $(addprefix -I,$(INCLUDES))



###############################################################################
#
# Main targets
#
###############################################################################

.PHONY: all
all: $(BUILDDIR)/bin/kernel


.PHONY: clean
clean:
	# TODO: revise
	$(RM) -r $(BUILDDIR)


$(BUILDDIR)/bin/kernel: $(OBJECTS) arch/$(TARGET_ARCH)/kernel.ld
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -T arch/$(TARGET_ARCH)/kernel.ld $(OBJECTS) -o $@



###############################################################################
#
# Dependencies
#
###############################################################################

SOURCES := \
	arch/$(TARGET_ARCH)/startup.s \
	arch/$(TARGET_ARCH)/cpu.cpp \
	arch/$(TARGET_ARCH)/libc.cpp \
	arch/$(TARGET_ARCH)/vmm.cpp \
	kernel.cpp \
	pmm.cpp

OBJECTS := $(SOURCES:%=$(BUILDDIR)/%.o)

DEPENDENCIES := $(OBJECTS:%.o=%.d)

-include $(DEPENDENCIES)



###############################################################################
#
# Rules
#
###############################################################################

# Reset the lists of suffixes we know how to build
.SUFFIXES:
.SUFFIXES: .c .cpp .h. .hpp .s .o

$(BUILDDIR)/%.s.o $(BUILDDIR)/%.s.d: %.s
	@mkdir -p $(dir $@)
	$(AS) -Wa,--MD,$@ $(ASFLAGS) $(CPPFLAGS) -c $< -o $(@:%.d=%.o)

$(BUILDDIR)/%.c.o $(BUILDDIR)/%.c.d: %.c
	@mkdir -p $(dir $@)
	$(CC) -MMD $(CFLAGS) $(CPPFLAGS) -c $< -o $(@:%.d=%.o)

$(BUILDDIR)/%.cpp.o $(BUILDDIR)/%.cpp.d: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) -MMD $(CXXFLAGS) $(CPPFLAGS) -c $< -o $(@:%.d=%.o)

$(BUILDDIR)/%.cc.o $(BUILDDIR)/%.cpp.d: %.cc
	@mkdir -p $(dir $@)
	$(CXX) -MMD $(CXXFLAGS) $(CPPFLAGS) -c $< -o $(@:%.d=%.o)
