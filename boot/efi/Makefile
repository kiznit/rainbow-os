# Copyright (c) 2015, Thierry Tremblay
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

ifeq ($(TARGET_ARCH),ia32)
TOOLPREFIX := i686-w64-mingw32-
EFI_TARGET_FILENAME := bootia32.efi
EFI_TARGET_FORMAT := efi-app-ia32
EFI_ENTRY_POINT := _efi_main
else ifeq ($(TARGET_ARCH),x86_64)
TOOLPREFIX := x86_64-w64-mingw32-
EFI_TARGET_FILENAME := bootx64.efi
EFI_TARGET_FORMAT := efi-app-x86_64
EFI_ENTRY_POINT := efi_main
else
$(error Invalid value: TARGET_ARCH = $(TARGET_ARCH))
endif


BUILDDIR ?= build/$(TARGET_ARCH)

include ../../Makefile.common

GLOBAL_INCLUDES := \
	. \
	../common \
	../../include \
	../../libc/include \
	../../third_party/gnu-efi/inc \
	../../third_party/gnu-efi/inc/$(TARGET_ARCH) \
	../../third_party/gnu-efi/inc/protocol

DEFINES := HAVE_USE_MS_ABI ARCH=$(TARGET_ARCH)

MODULES := . ../common ../../libc


$(eval $(call load-modules,$(MODULES)))


libgcc_path := $(shell $(CC) -print-file-name=libgcc.a)


CPPFLAGS	:= $(addprefix -I,$(GLOBAL_INCLUDES)) $(addprefix -D,$(DEFINES))
CFLAGS		:= $(CFLAGS) -fshort-wchar -Wno-format
CXXFLAGS	:= $(CXXFLAGS) -fshort-wchar -Wno-format
LDFLAGS		:= $(LDFLAGS) -shared -e $(EFI_ENTRY_POINT) --subsystem=10 --image-base=0x100000

ifeq ($(TARGET_ARCH),ia32)
CFLAGS		:= $(CFLAGS) -march=i386
CXXFLAGS	:= $(CXXFLAGS) -march=i386
else ifeq ($(TARGET_ARCH),x86_64)
CFLAGS		:= $(CFLAGS) -mno-mmx -mno-sse
CXXFLAGS	:= $(CXXFLAGS) -mno-mmx -mno-sse
endif



.PHONY all:
all: $(BUILDDIR)/bin/$(EFI_TARGET_FILENAME)

$(BUILDDIR)/bin/$(EFI_TARGET_FILENAME): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) $(OBJECTS) $(libgcc_path) -o $@


.PHONY: clean
clean:
	$(RM) -r $(BUILDDIR)


-include $(DEPENDENCIES)
