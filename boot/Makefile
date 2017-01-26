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

TARGET_MACHINE ?= efi

ifeq ($(TARGET_MACHINE),efi)

	# Machine with EFI firmware
	TARGET_ARCH ?= x86_64

	ifeq ($(TARGET_ARCH),ia32)

		TOOLPREFIX := i686-w64-mingw32-
		ARCH_FLAGS := -march=i386 -mno-mmx -mno-sse
		TARGET_FILENAME := bootia32.efi

	else ifeq ($(TARGET_ARCH),x86_64)

		TOOLPREFIX := x86_64-w64-mingw32-
		ARCH_FLAGS := -mno-mmx -mno-sse
		TARGET_FILENAME := bootx64.efi

	else

		$(error Unsupported architecture for EFI: $(TARGET_ARCH))

	endif

else ifeq ($(TARGET_MACHINE),multiboot)

	# Machine with multiboot loader
	TARGET_ARCH ?= ia32
	ARCH_FLAGS := -march=i386 -mno-mmx -mno-sse
	TARGET_FILENAME := multiboot

else ifeq ($(TARGET_MACHINE),raspi)

	# Raspberry 1
	TARGET_ARCH := arm
	ARCH_FLAGS := -march=armv6kz -mtune=arm1176jzf-s -mfloat-abi=hard -mfpu=vfp
	TARGET_FILENAME := kernel.img

else ifeq ($(TARGET_MACHINE),raspi2)

	# Raspberry 2
	TARGET_ARCH := arm
	ARCH_FLAGS := -march=armv7-a -mtune=cortex-a7 -mfloat-abi=hard -mfpu=neon-vfpv4
	TARGET_FILENAME := kernel7.img

else ifeq ($(TARGET_MACHINE),raspi3)

	# Raspberry 3
	TARGET_ARCH := arm
	ARCH_FLAGS := -march=armv8-a+crc -mtune=cortex-a53 -mfloat-abi=hard -mfpu=crypto-neon-fp-armv8
	TARGET_FILENAME := kernel7.img

else

	$(error Unknown TARGET_MACHINE: $(TARGET_MACHINE))

endif



BUILDDIR ?= build/$(TARGET_MACHINE)/$(TARGET_ARCH)



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

CC       := $(TOOLPREFIX)gcc
CXX      := $(TOOLPREFIX)gcc
LD       := $(TOOLPREFIX)ld
AS       := $(TOOLPREFIX)gcc
OBJCOPY  := $(TOOLPREFIX)objcopy

CFLAGS   := $(ARCH_FLAGS) -O2 -ffreestanding -fbuiltin -Wall -Wextra -Werror -std=gnu99
CXXFLAGS := $(ARCH_FLAGS) -O2 -ffreestanding -fbuiltin -Wall -Wextra -Werror -std=gnu++11 -fno-exceptions -fno-rtti
ASFLAGS  := $(ARCH_FLAGS) -O2 -ffreestanding -fbuiltin -Wall -Wextra -Werror
LDFLAGS  := -nostdlib --warn-common --fatal-warnings --no-undefined -T $(TARGET_MACHINE)/boot.ld



###############################################################################
#
# Targets
#
###############################################################################

.PHONY: all
all: $(BUILDDIR)/$(TARGET_FILENAME)

$(BUILDDIR)/$(TARGET_FILENAME): $(OBJECTS) $(TARGET_MACHINE)/boot.ld
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
