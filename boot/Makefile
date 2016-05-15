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


TARGET_ARCH ?= x86_64
TARGET_BOOTLOADER ?= efi
BUILDDIR ?= build/$(TARGET_ARCH)/$(TARGET_BOOTLOADER)

# Toolchain
ifeq ($(TARGET_BOOTLOADER),efi)
	ifeq ($(TARGET_ARCH),ia32)
		TOOLPREFIX := i686-w64-mingw32-
		TARGET_FILENAME := bootia32.efi
		EFI_ENTRY_POINT := _efi_main
	else ifeq ($(TARGET_ARCH),x86_64)
		TOOLPREFIX := x86_64-w64-mingw32-
		TARGET_FILENAME := bootx64.efi
		EFI_ENTRY_POINT := efi_main
	else
		$(error Invalid value: TARGET_ARCH = $(TARGET_ARCH))
	endif
else ifeq ($(TARGET_BOOTLOADER),multiboot)
	ifeq ($(TARGET_ARCH),ia32)
		TARGET_FILENAME := multiboot
	else
		$(error Invalid value: TARGET_ARCH = $(TARGET_ARCH))
	endif
else
	$(error Invalid value: TARGET_BOOTLOADER = $(TARGET_BOOTLOADER))
endif


include ../mk/Makefile.common

MODULES := . libc libc++

ifeq ($(TARGET_BOOTLOADER),efi)
MODULES += efi
else ifeq ($(TARGET_BOOTLOADER),multiboot)
MODULES += multiboot vgaconsole
endif

$(eval $(call load-modules,$(MODULES)))


# Target arch customization
ifeq ($(TARGET_ARCH),ia32)
CFLAGS		:= $(CFLAGS) -march=i386
CXXFLAGS	:= $(CXXFLAGS) -march=i386
else ifeq ($(TARGET_ARCH),x86_64)
CFLAGS		:= $(CFLAGS) -mno-mmx -mno-sse
CXXFLAGS	:= $(CXXFLAGS) -mno-mmx -mno-sse
endif

# Target bootloader customization
ifeq ($(TARGET_BOOTLOADER),efi)
CFLAGS		:= $(CFLAGS) -fshort-wchar -Wno-format
CXXFLAGS	:= $(CXXFLAGS) -fshort-wchar -Wno-format
LDFLAGS		:= $(LDFLAGS) -shared -e $(EFI_ENTRY_POINT) --subsystem=10 --image-base=0x100000
else ifeq ($(TARGET_BOOTLOADER),multiboot)
LDFLAGS		:= $(LDFLAGS) -T multiboot/multiboot.ld
endif

libgcc_path := $(shell $(CC) -print-file-name=libgcc.a)


.PHONY all:
all: $(BUILDDIR)/bin/$(TARGET_FILENAME)

$(BUILDDIR)/bin/$(TARGET_FILENAME): $(OBJECTS) multiboot/multiboot.ld
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) $(OBJECTS) $(libgcc_path) -o $@


.PHONY: clean
clean:
	$(RM) -r $(BUILDDIR)


-include $(DEPENDENCIES)
