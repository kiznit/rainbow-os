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

TARGET_ARCH := arm
TARGET_MACHINE ?= raspi2
BUILDDIR ?= build/$(TARGET_MACHINE)



###############################################################################
#
# Toolchain
#
###############################################################################

ifeq ($(TARGET_MACHINE),raspi)
	TARGET_FILENAME := kernel.img
else
	TARGET_FILENAME := kernel7.img
endif

TOOLPREFIX ?= arm-none-eabi-

CC	:= $(TOOLPREFIX)gcc
CXX	:= $(TOOLPREFIX)g++
LD	:= $(TOOLPREFIX)ld
AS	:= $(TOOLPREFIX)as
OBJCOPY := $(TOOLPREFIX)objcopy

ifeq ($(TARGET_MACHINE),raspi)
	# Processor is BCM2835
	ARCH_FLAGS := -march=armv6kz -mtune=arm1176jzf-s -mfloat-abi=hard -mfpu=vfpv2
else ifeq ($(TARGET_MACHINE),raspi2)
	# Processor is BCM2836
	ARCH_FLAGS := -march=armv7-a -mtune=cortex-a7 -mfloat-abi=hard -mfpu=neon-vfpv4
else ifeq ($(TARGET_MACHINE),raspi3)
	# Processor is BCM2837
	ARCH_FLAGS := -march=armv8-a+crc -mtune=cortex-a53 -mfloat-abi=hard -mfpu=crypto-neon-fp-armv8
else
	$(error Unknown TARGET_MACHINE: $(TARGET_MACHINE))
endif

CFLAGS		:= $(ARCH_FLAGS) -O2 -ffreestanding -fbuiltin -Wall -Wextra -Werror -std=gnu99
CXXFLAGS	:= $(ARCH_FLAGS) -O2 -ffreestanding -fbuiltin -Wall -Wextra -Werror -std=gnu++11 -fno-exceptions -fno-rtti
ASFLAGS		:= $(ARCH_FLAGS) -O2 -ffreestanding -fbuiltin -Wall -Wextra -Werror
LDFLAGS		:= -nostdlib --warn-common --fatal-warnings --no-undefined -T raspi.ld



###############################################################################
#
# Source
#
###############################################################################

CPPFLAGS :=

SOURCES := entry.s raspi.cpp

OBJECTS := $(SOURCES:%=$(BUILDDIR)/%.o)

DEPENDENCIES := $(OBJECTS:%.o=%.d)



###############################################################################
#
# Targets
#
###############################################################################

.PHONY: all
all: $(BUILDDIR)/$(TARGET_FILENAME)

$(BUILDDIR)/$(TARGET_FILENAME:%.img=%.elf): $(OBJECTS) raspi.ld
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) $(OBJECTS) -o $@

$(BUILDDIR)/$(TARGET_FILENAME): $(BUILDDIR)/$(TARGET_FILENAME:%.img=%.elf)
	@mkdir -p $(dir $@)
	$(OBJCOPY) $< -O binary $@



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
