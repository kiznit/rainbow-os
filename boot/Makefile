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
# Configuration
#
###############################################################################

ifeq ($(TARGET_MACHINE),efi)
	# We require mingw to properly generate PE executables for UEFI
	ifeq ($(TARGET_ARCH),ia32)
		TOOLPREFIX := i686-w64-mingw32-
	else ifeq ($(TARGET_ARCH),x86_64)
		TOOLPREFIX := x86_64-w64-mingw32-
	endif
else ifeq ($(TARGET_MACHINE),bios)
	# BIOS is always 32-bit x86, enforce this
	TARGET_ARCH := ia32
endif



###############################################################################
#
# Source
#
###############################################################################

SOURCES := \
	boot.cpp \
	elf.cpp \
	memory.cpp \
	graphics/graphicsconsole.cpp \
	graphics/pixels.cpp \
	graphics/surface.cpp \
	graphics/vgafont.cpp \
	video/display.cpp \
	video/edid.cpp

ifeq ($(TARGET_MACHINE),efi)

	SOURCES += \
		graphics/vgacolors.cpp \
		platform/efi/efi.cpp \
		platform/efi/eficonsole.cpp \
		platform/efi/efidisplay.cpp \
		platform/efi/libc.cpp

else ifeq ($(TARGET_MACHINE),bios)

	SOURCES += \
		graphics/vgacolors.cpp \
		platform/bios/bios.s \
		platform/bios/entry.s \
		platform/bios/bios.cpp \
		platform/bios/libc.cpp \
		platform/bios/vgaconsole.cpp \
		platform/bios/vbe.cpp \
		platform/bios/vbedisplay.cpp

	LINK_SCRIPT := platform/bios/bios.ld

else ifneq ($(filter $(TARGET_MACHINE),raspi raspi2 raspi3),)

	SOURCES += \
		platform/raspi/arm.cpp \
		platform/raspi/libc.cpp \
		platform/raspi/mailbox.cpp \
		platform/raspi/raspi.cpp \
		platform/raspi/$(TARGET_ARCH)/entry.s

	LINK_SCRIPT := platform/raspi/$(TARGET_ARCH)/raspi.ld

endif

INCLUDES += . $(SRCDIR)/headers $(SRCDIR)/third_party/dlmalloc

OBJECTS := $(SOURCES:%=$(BUILDDIR)/%.o)

DEPENDENCIES := $(OBJECTS:%.o=%.d)


# Grab pieces of libc for now

INCLUDES += $(SRCDIR)/libc/include

LIBC_SOURCES := \
	assert.c \
	stdio/printf.c \
	stdio/putchar.c \
	stdio/puts.c \
	string/memcpy.c \
	string/memset.c \
	string/strcmp.c \
	string/strlen.c \
	string/wcslen.c

LIBC_OBJECTS := $(LIBC_SOURCES:%=$(BUILDDIR)/libc/%.o)

LIBC_DEPENDENCIES := $(LIBC_OBJECTS:%.o=%.d)

OBJECTS += $(LIBC_OBJECTS)
DEPENDENCIES += $(LIBC_DEPENDENCIES)



###############################################################################
#
# Toolchain
#
###############################################################################

include $(SRCDIR)/mk/toolchain.mk

ifeq ($(TARGET_MACHINE),efi)
	# mingw doesn't support gnu17
	CFLAGS := $(subst $(CFLAGS),-std=gnu17,-std=gnu11)
endif

CFLAGS   += -ffreestanding -fbuiltin
CXXFLAGS += -ffreestanding -fbuiltin -fno-exceptions -fno-rtti
ASFLAGS  += -ffreestanding -fbuiltin
LDFLAGS  += -nostdlib

ifneq ($(LINK_SCRIPT),)
	LDFLAGS := $(LDFLAGS) -T $(LINK_SCRIPT)
endif



ifeq ($(TARGET_MACHINE),efi)

	# Machine with EFI firmware
	INCLUDES += $(SRCDIR)/third_party/uefi-headers/Include
	DEFINES += EFIAPI="__attribute__((ms_abi))" EFI_ARCH=$(TARGET_ARCH)
	CFLAGS += -fshort-wchar -Wno-format
	CXXFLAGS += -fshort-wchar -Wno-format

	ifeq ($(TARGET_ARCH),ia32)

		ARCH_FLAGS := -march=i386 -mno-mmx -mno-sse
		LDFLAGS += -shared -e _efi_main --subsystem=10 --image-base=0x100000
		INCLUDES += $(SRCDIR)/third_party/uefi-headers/Include/Ia32

	else ifeq ($(TARGET_ARCH),x86_64)

		ARCH_FLAGS := -mno-mmx -mno-sse
		LDFLAGS += -shared -e efi_main --subsystem=10 --image-base=0x100000
		INCLUDES += $(SRCDIR)/third_party/uefi-headers/Include/X64

	else

		$(error Unsupported architecture for EFI: $(TARGET_ARCH))

	endif

else ifeq ($(TARGET_MACHINE),bios)

	# Machine with BIOS firmware
	ARCH_FLAGS := -march=i386 -mno-mmx -mno-sse
	INCLUDES += $(SRCDIR)/third_party/multiboot

endif


ifeq ($(TARGET_ARCH),aarch64)
	# Unaligned access isn't allowed until the MMU is turned on, so don't allow it for the bootloader
	ARCH_FLAGS += -mstrict-align
endif



###############################################################################
#
# Targets
#
###############################################################################

LIBGCC := $(shell $(CC) $(ARCH_FLAGS) $(CFLAGS) -print-file-name=libgcc.a)


.PHONY: all
all: $(BUILDDIR)/bootloader

ifneq ($(filter $(TARGET_MACHINE),raspi raspi2 raspi3),)

$(BUILDDIR)/bootloader.elf: $(OBJECTS) $(LINK_SCRIPT)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) $(OBJECTS) $(LIBGCC) -o $@

$(BUILDDIR)/bootloader: $(BUILDDIR)/bootloader.elf
	@mkdir -p $(dir $@)
	$(OBJCOPY) $< -O binary $@

else

$(BUILDDIR)/bootloader: $(OBJECTS) $(LINK_SCRIPT)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) $(OBJECTS) $(LIBGCC) -o $@

endif


.PHONY: clean
clean:
	@test -n "$(BUILDDIR)" && rm -rf $(OBJECTS) $(DEPENDENCIES) $(BUILDDIR)/bootloader



###############################################################################
#
# Rules
#
###############################################################################

include $(SRCDIR)/mk/rules.mk


ifneq ($(MAKECMDGOALS),clean)
-include $(DEPENDENCIES)
endif
