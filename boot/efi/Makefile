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

SHELL := /bin/bash


###############################################################################
#
# Configuration
#
###############################################################################

TARGET_ARCH ?= x86_64
BUILDDIR ?= build/$(TARGET_ARCH)



###############################################################################
#
# Toolchain
#
###############################################################################

ifeq ($(TARGET_ARCH),ia32)
	TOOLPREFIX := i686-w64-mingw32-
	TARGET_FILENAME := bootia32.efi
	EFI_ENTRY_POINT := _efi_main
else ifeq ($(TARGET_ARCH),x86_64)
	TOOLPREFIX := x86_64-w64-mingw32-
	TARGET_FILENAME := bootx64.efi
	EFI_ENTRY_POINT := efi_main
else
	$(error Unsupported value: TARGET_ARCH = $(TARGET_ARCH))
endif

CC	:= $(TOOLPREFIX)gcc
CXX	:= $(TOOLPREFIX)g++
LD	:= $(TOOLPREFIX)ld
AS	:= $(TOOLPREFIX)as

ifeq ($(TARGET_ARCH),ia32)
ARCH_FLAGS	:= -march=i386 -mno-mmx -mno-sse
else ifeq ($(TARGET_ARCH),x86_64)
ARCH_FLAGS	:= -mno-mmx -mno-sse
endif

CFLAGS		:= $(ARCH_FLAGS) -O2 -ffreestanding -fbuiltin -Wall -Wextra -Werror -fshort-wchar -std=gnu99
CXXFLAGS	:= $(ARCH_FLAGS) -O2 -ffreestanding -fbuiltin -Wall -Wextra -Werror -fshort-wchar -std=gnu++11 -fno-exceptions -fno-rtti
ASFLAGS		:= $(ARCH_FLAGS) -O2 -ffreestanding -fbuiltin -Wall -Wextra -Werror
LDFLAGS		:= -nostdlib --warn-common --fatal-warnings --no-undefined -shared -e $(EFI_ENTRY_POINT) --subsystem=10 --image-base=0x100000



###############################################################################
#
# Source
#
###############################################################################

CPPFLAGS := -DEFIAPI="__attribute__((ms_abi))" -I../../third_party/uefi-headers/Include

ifeq ($(TARGET_ARCH),ia32)
CPPFLAGS := $(CPPFLAGS) -I../../third_party/uefi-headers/Include/Ia32
else ifeq ($(TARGET_ARCH),x86_64)
CPPFLAGS := $(CPPFLAGS) -I../../third_party/uefi-headers/Include/X64
endif


SOURCES := entry.cpp

OBJECTS := $(SOURCES:%=$(BUILDDIR)/%.o)

DEPENDENCIES := $(OBJECTS:%.o=%.d)



###############################################################################
#
# Targets
#
###############################################################################

.PHONY: all
all: $(BUILDDIR)/$(TARGET_FILENAME)

$(BUILDDIR)/$(TARGET_FILENAME): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) $(OBJECTS) -o $@



###############################################################################
#
# Rules
#
###############################################################################

.SUFFIXES:
.SUFFIXES: .c .cpp .h. .hpp .s .o

$(BUILDDIR)/%.c.o $(BUILDDIR)/%.c.d: %.c
	@mkdir -p $(dir $@)
	$(CC) -MMD $(CFLAGS) $(CPPFLAGS) -c $< -o $(@:%.d=%.o)

$(BUILDDIR)/%.cpp.o $(BUILDDIR)/%.cpp.d: %.cpp
	@mkdir -p $(dir $@)
	$(CC) -MMD $(CXXFLAGS) $(CPPFLAGS) -c $< -o $(@:%.d=%.o)

$(BUILDDIR)/%.s.o $(BUILDDIR)/%.s.d: %.s
	@mkdir -p $(dir $@)
	$(CC) -Wa,--MD,$@ $(ASFLAGS) $(CPPFLAGS) -c $< -o $(@:%.d=%.o)


-include $(DEPENDENCIES)
