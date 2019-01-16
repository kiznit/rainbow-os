# Copyright (c) 2018, Thierry Tremblay
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

MKFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
TOPDIR := $(dir $(MKFILE_PATH))

include $(TOPDIR)/defaults.mk

BUILDDIR := $(TOPDIR)/build/$(ARCH)


###############################################################################
#
# Targets
#
###############################################################################

.PHONY: all
all: boot kernel


.PHONY: clean
clean:
	$(RM) -rf $(BUILDDIR)


.PHONY: boot
boot:
	mkdir -p $(BUILDDIR)/boot && cd $(BUILDDIR)/boot && $(MAKE) -f $(TOPDIR)/boot/Makefile


.PHONY: kernel
kernel:
	mkdir -p $(BUILDDIR)/kernel && cd $(BUILDDIR)/kernel && $(MAKE) -f $(TOPDIR)/kernel/Makefile



# Build an EFI bootable image
ifeq ($(ARCH),ia32)
EFI_BOOTLOADER := bootia32.efi
else ifeq ($(ARCH),x86_64)
EFI_BOOTLOADER := bootx64.efi
endif

.PHONY: image
image: boot kernel
	@ $(RM) -rf $(BUILDDIR)/image
	# bootloader
	mkdir -p $(BUILDDIR)/image/efi/rainbow
	cp $(BUILDDIR)/boot/boot.efi $(BUILDDIR)/image/efi/rainbow/$(EFI_BOOTLOADER)
	# Fallback location for removal media (/efi/boot)
	mkdir -p $(BUILDDIR)/image/efi/boot
	cp $(BUILDDIR)/boot/boot.efi $(BUILDDIR)/image/efi/boot/$(EFI_BOOTLOADER)
	# Kernel
	cp $(BUILDDIR)/kernel/kernel $(BUILDDIR)/image/efi/rainbow/
	# Build IMG
	dd if=/dev/zero of=$(BUILDDIR)/rainbow-uefi.img bs=1M count=33
	mkfs.vfat $(BUILDDIR)/rainbow-uefi.img -F32
	mcopy -s -i $(BUILDDIR)/rainbow-uefi.img $(BUILDDIR)/image/* ::


# Run the image under QEMU
ifeq ($(ARCH),ia32)
QEMU ?= qemu-system-i386
FIRMWARE ?= $(TOPDIR)/third_party/tianocore/ovmf-ia32-r15214.fd
else ifeq ($(ARCH),x86_64)
QEMU ?= qemu-system-x86_64
FIRMWARE ?= $(TOPDIR)/third_party/tianocore/ovmf-x64-r15214.fd
endif

QEMUFLAGS ?= -m 8G
QEMUFLAGS += -bios $(FIRMWARE) -drive format=raw,file=$(BUILDDIR)/rainbow-uefi.img

.PHONY: run
run: image
	$(QEMU) $(QEMUFLAGS)

.PHONY: debug
debug: image
	$(QEMU) $(QEMUFLAGS) -s -S -debugcon file:debug.log -global isa-debugcon.iobase=0x402
