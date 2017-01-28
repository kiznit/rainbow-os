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



###############################################################################
#
# Configuration
#
###############################################################################

ifeq ($(TARGET_MACHINE),raspi)
	# Processor is BCM2835 (ARMv6)
	TARGET_ARCH := arm
	ARCH_FLAGS := -march=armv6kz -mtune=arm1176jzf-s -mfloat-abi=hard -mfpu=vfp
else ifeq ($(TARGET_MACHINE),raspi2)
	# Processor is BCM2836 (ARMv7)
	TARGET_ARCH := arm
	ARCH_FLAGS := -march=armv7-a -mtune=cortex-a7 -mfloat-abi=hard -mfpu=neon-vfpv4
else ifeq ($(TARGET_MACHINE),raspi3)
	# Processor is BCM2837 (ARMv8)
	ifeq ($(TARGET_ARCH),aarch64)
		$(error Unsupported value: TARGET_ARCH = $(TARGET_ARCH))
	else
		TARGET_ARCH := arm
		ARCH_FLAGS := -march=armv8-a+crc -mtune=cortex-a53 -mfloat-abi=hard -mfpu=crypto-neon-fp-armv8
	endif
else
	TARGET_ARCH ?= x86_64

	ifeq ($(TARGET_ARCH),ia32)
		ARCH_FLAGS := -march=i686
	else ifeq ($(TARGET_ARCH),x86_64)
		ARCH_FLAGS :=
	else
		$(error Unsupported value: TARGET_ARCH = $(TARGET_ARCH))
	endif
endif


###############################################################################
#
# Toolchain
#
###############################################################################

# For now we assume that we always want to use a cross-compiler.
ifndef TOOLPREFIX
	ifeq ($(TARGET_ARCH),ia32)
		TOOLPREFIX ?= i686-elf-
	else ifeq ($(TARGET_ARCH),x86_64)
		TOOLPREFIX ?= x86_64-elf-
	else ifeq ($(TARGET_ARCH),arm)
		TOOLPREFIX ?= arm-none-eabi-
	else ifeq ($(TARGET_ARCH),aarch64)
		TOOLPREFIX ?= aarch64-none-elf-
	endif
endif

CC			:= $(TOOLPREFIX)gcc
CXX			:= $(TOOLPREFIX)gcc
LD			:= $(TOOLPREFIX)ld
AS			:= $(TOOLPREFIX)gcc
OBJCOPY		:= $(TOOLPREFIX)objcopy

DEFINES		+=
INCLUDES	+=
CPPFLAGS	+= $(addprefix -D,$(DEFINES)) $(addprefix -I,$(INCLUDES))

CFLAGS		+= -O2 -Wall -Wextra -Werror -std=gnu99
CXXFLAGS	+= -O2 -Wall -Wextra -Werror -std=gnu++11
ASFLAGS		+= -O2 -Wall -Wextra -Werror

LIBRARIES	+=
LDFLAGS		+= --warn-common --fatal-warnings --no-undefined
