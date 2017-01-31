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
else ifeq ($(TARGET_MACHINE),multiboot)
	# Multiboot is always 32-bit x86, enforce this
	TARGET_ARCH := ia32
endif



###############################################################################
#
# Source
#
###############################################################################

SOURCES :=

ifeq ($(TARGET_MACHINE),efi)

	SOURCES += efi/efi.cpp

else ifeq ($(TARGET_MACHINE),multiboot)

	SOURCES += multiboot/entry.s multiboot/multiboot.cpp multiboot/vgaconsole.cpp
	LINK_SCRIPT := multiboot/multiboot.ld

else ifneq ($(filter $(TARGET_MACHINE),raspi raspi2 raspi3),)

	SOURCES += raspi/entry.s raspi/raspi.cpp
	LINK_SCRIPT := raspi/raspi.ld

endif

INCLUDES += .

OBJECTS := $(SOURCES:%=$(BUILDDIR)/%.o)

DEPENDENCIES := $(OBJECTS:%.o=%.d)


# Grab pieces of libc for now

INCLUDES += $(SRCDIR)/libc/include

LIBC_SOURCES := \
	stdio/printf.c \
	stdio/putchar.c \
	stdio/puts.c \
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
	CFLAGS += -fshort-wchar
	CXXFLAGS += -fshort-wchar

	ifeq ($(TARGET_ARCH),ia32)

		ARCH_FLAGS := -march=i386 -mno-mmx -mno-sse
		LDFLAGS += -shared -e _efi_main --subsystem=10 --image-base=0x100000
		TARGET_FILENAME := bootia32.efi
		INCLUDES += $(SRCDIR)/third_party/uefi-headers/Include/Ia32

	else ifeq ($(TARGET_ARCH),x86_64)

		ARCH_FLAGS := -mno-mmx -mno-sse
		LDFLAGS += -shared -e efi_main --subsystem=10 --image-base=0x100000
		TARGET_FILENAME := bootx64.efi
		INCLUDES += $(SRCDIR)/third_party/uefi-headers/Include/X64

	else

		$(error Unsupported architecture for EFI: $(TARGET_ARCH))

	endif

else ifeq ($(TARGET_MACHINE),multiboot)

	# Machine with multiboot loader
	ARCH_FLAGS := -march=i386 -mno-mmx -mno-sse
	TARGET_FILENAME := rainbow
	INCLUDES += $(SRCDIR)/third_party/multiboot

else ifeq ($(TARGET_MACHINE),raspi)

	# Raspberry 1
	TARGET_FILENAME := kernel.img

else ifeq ($(TARGET_MACHINE),raspi2)

	# Raspberry 2
	TARGET_FILENAME := kernel7.img

else ifeq ($(TARGET_MACHINE),raspi3)

	# Raspberry 3
	TARGET_FILENAME := kernel7.img

else

	$(error Unknown TARGET_MACHINE: $(TARGET_MACHINE))

endif



###############################################################################
#
# Targets
#
###############################################################################

LIBGCC := $(shell $(CC) $(CFLAGS) -print-file-name=libgcc.a)


.PHONY: all
all: $(BUILDDIR)/$(TARGET_FILENAME)

ifneq ($(filter $(TARGET_MACHINE),raspi raspi2 raspi3),)

$(BUILDDIR)/$(TARGET_FILENAME:%.img=%.elf): $(OBJECTS) $(LINK_SCRIPT)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) $(OBJECTS) $(LIBGCC) -o $@

$(BUILDDIR)/$(TARGET_FILENAME): $(BUILDDIR)/$(TARGET_FILENAME:%.img=%.elf)
	@mkdir -p $(dir $@)
	$(OBJCOPY) $< -O binary $@

else

$(BUILDDIR)/$(TARGET_FILENAME): $(OBJECTS) $(LINK_SCRIPT)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) $(OBJECTS) $(LIBGCC) -o $@

endif


.PHONY: clean
clean:
	@test -n "$(BUILDDIR)" && rm -rf $(OBJECTS) $(DEPENDENCIES) $(BUILDDIR)/$(TARGET_FILENAME)



###############################################################################
#
# Rules
#
###############################################################################

include $(SRCDIR)/mk/rules.mk


ifneq ($(MAKECMDGOALS),clean)
-include $(DEPENDENCIES)
endif
