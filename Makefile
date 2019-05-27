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

include $(TOPDIR)/mk/defaults.mk

BUILDDIR := $(TOPDIR)/build/$(ARCH)


###############################################################################
#
# Machine
#
###############################################################################

MACHINE ?= efi

IMAGE = $(MACHINE)_image


###############################################################################
#
# Targets
#
###############################################################################

.PHONY: all
all: boot kernel


.PHONY: clean
clean:
	$(RM) -r $(BUILDDIR)


.PHONY: boot
boot:
	mkdir -p $(BUILDDIR)/boot && cd $(BUILDDIR)/boot && MACHINE=$(MACHINE) $(MAKE) -f $(TOPDIR)/boot/Makefile


.PHONY: kernel
kernel:
	mkdir -p $(BUILDDIR)/kernel && cd $(BUILDDIR)/kernel && $(MAKE) -f $(TOPDIR)/kernel/Makefile

.PHONY: image
image: $(IMAGE)



###############################################################################
#
# EFI Image
#
###############################################################################

ifeq ($(ARCH),ia32)
EFI_BOOTLOADER := bootia32.efi
else ifeq ($(ARCH),x86_64)
EFI_BOOTLOADER := bootx64.efi
endif

.PHONY: efi_image
efi_image: boot kernel
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
	dd if=/dev/zero of=$(BUILDDIR)/rainbow.img bs=1M count=33
	mkfs.vfat $(BUILDDIR)/rainbow.img -F32
	mcopy -s -i $(BUILDDIR)/rainbow.img $(BUILDDIR)/image/* ::


###############################################################################
#
# BIOS Image
#
###############################################################################

.PHONY: bios_image
bios_image: boot kernel
	@ $(RM) -rf $(BUILDDIR)/image
	# Grub boot files
	mkdir -p $(BUILDDIR)/image/boot/grub
	cp $(TOPDIR)/boot/machine/bios/grub.cfg $(BUILDDIR)/image/boot/grub/grub.cfg
	# bootloader
	mkdir -p $(BUILDDIR)/image/boot/rainbow
	cp $(BUILDDIR)/boot/bootloader $(BUILDDIR)/image/boot/rainbow/
	# Kernel
	cp $(BUILDDIR)/kernel/kernel $(BUILDDIR)/image/boot/rainbow/
	# Build ISO image
	grub-mkrescue -d /usr/lib/grub/i386-pc -o $(BUILDDIR)/rainbow.img $(BUILDDIR)/image


###############################################################################
#
# Run image under QEMU
#
###############################################################################

QEMUFLAGS ?= -m 8G

ifeq ($(ARCH),ia32)
	QEMU ?= qemu-system-i386
	ifeq ($(MACHINE),efi)
		FIRMWARE ?= $(TOPDIR)/third_party/tianocore/ovmf-ia32-r15214.fd
	endif
else ifeq ($(ARCH),x86_64)
	QEMU ?= qemu-system-x86_64
	ifeq ($(MACHINE),efi)
		FIRMWARE ?= $(TOPDIR)/third_party/tianocore/ovmf-x64-r15214.fd
	endif
endif

ifneq ($(FIRMWARE),)
	QEMUFLAGS += -bios $(FIRMWARE)
endif

QEMUFLAGS += -drive format=raw,file=$(BUILDDIR)/rainbow.img

.PHONY: run-qemu
run-qemu: $(IMAGE)
	$(QEMU) -monitor stdio $(QEMUFLAGS)

.PHONY: debug
debug: $(IMAGE)
	$(QEMU) $(QEMUFLAGS) -s -S -debugcon file:debug.log -global isa-debugcon.iobase=0x402

.PHONY: run
run: run-qemu


###############################################################################
#
# Run image under Bochs
#
###############################################################################

BOCHS ?= bochs

.PHONY: run-bochs
run-bochs: $(IMAGE)
	ARCH=$(ARCH) $(BOCHS) -f $(TOPDIR)/.bochsrc -q
