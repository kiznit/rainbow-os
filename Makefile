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

ROOTDIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

SRCDIR ?= $(ROOTDIR)

BUILDDIR ?= $(CURDIR)/build
BINDIR ?= $(CURDIR)/bin

QEMUFLAGS ?= -m 8G


.PHONY: all
all: bios-image efi-image


.PHONY: clean
clean:
	$(RM) -r $(BUILDDIR)
	$(RM) -r $(BINDIR)



###############################################################################
# Boot loaders
###############################################################################

.PHONY: efi_ia32
efi_ia32:
	$(MAKE) TARGET_ARCH=ia32 BUILDDIR=$(BUILDDIR)/ia32/efi -C $(SRCDIR)/boot/efi

.PHONY: efi_x86_64
efi_x86_64:
	$(MAKE) TARGET_ARCH=x86_64 BUILDDIR=$(BUILDDIR)/x86_64/efi -C $(SRCDIR)/boot/efi

.PHONY: multiboot_ia32
multiboot_ia32:
	$(MAKE) TARGET_ARCH=ia32 BUILDDIR=$(BUILDDIR)/ia32/multiboot -C $(SRCDIR)/boot/multiboot



###############################################################################
# Launcher
###############################################################################

.PHONE: launcher_ia32
launcher_ia32:
	$(MAKE) TARGET_ARCH=ia32 BUILDDIR=$(BUILDDIR)/ia32/launcher -C $(SRCDIR)/boot/launcher



###############################################################################
# Kernels
###############################################################################

.PHONY: kernel_ia32
kernel_ia32:
	$(MAKE) TARGET_ARCH=ia32 BUILDDIR=$(BUILDDIR)/ia32/kernel -C $(SRCDIR)/kernel

.PHONY: kernel_x86_64
kernel_x86_64:
	$(MAKE) TARGET_ARCH=x86_64 BUILDDIR=$(BUILDDIR)/x86_64/kernel -C $(SRCDIR)/kernel



###############################################################################
# BIOS image
###############################################################################

.PHONY: bios-image
bios-image: multiboot_ia32 launcher_ia32 kernel_ia32 kernel_x86_64
	$(RM) -r $(BUILDDIR)/bios-image
	mkdir -p $(BUILDDIR)/bios-image/boot/grub
	cp $(BUILDDIR)/ia32/multiboot/bin/multiboot $(BUILDDIR)/bios-image/boot/rainbow_multiboot
	cp $(SRCDIR)/iso/grub.cfg $(BUILDDIR)/bios-image/boot/grub/grub.cfg
	mkdir -p $(BUILDDIR)/bios-image/rainbow
	cp $(BUILDDIR)/ia32/launcher/bin/launcher $(BUILDDIR)/bios-image/rainbow/launcher
	cp $(BUILDDIR)/ia32/kernel/bin/kernel $(BUILDDIR)/bios-image/rainbow/kernel_ia32
	cp $(BUILDDIR)/x86_64/kernel/bin/kernel $(BUILDDIR)/bios-image/rainbow/kernel_x86_64
	mkdir -p $(BINDIR)
	grub-mkrescue -o $(BINDIR)/rainbow-bios.iso $(BUILDDIR)/bios-image

.PHONY: multiboot
multiboot: $(BUILDDIR)/x86/multiboot/Makefile
	$(MAKE) -C $(BUILDDIR)/x86/multiboot



###############################################################################
# EFI image
###############################################################################

.PHONY: efi-image
efi-image: efi_ia32 efi_x86_64 launcher_ia32 kernel_ia32 kernel_x86_64
	$(RM) -r $(BUILDDIR)/efi-image
	mkdir -p $(BUILDDIR)/efi-image/efi/boot
	cp $(BUILDDIR)/ia32/efi/bin/bootia32.efi $(BUILDDIR)/efi-image/efi/boot
	cp $(BUILDDIR)/x86_64/efi/bin/bootx64.efi $(BUILDDIR)/efi-image/efi/boot
	mkdir -p $(BUILDDIR)/efi-image/rainbow
	cp $(BUILDDIR)/ia32/efi/bin/bootia32.efi $(BUILDDIR)/efi-image/rainbow
	cp $(BUILDDIR)/x86_64/efi/bin/bootx64.efi $(BUILDDIR)/efi-image/rainbow
	cp $(BUILDDIR)/ia32/launcher/bin/launcher $(BUILDDIR)/efi-image/rainbow/launcher
	cp $(BUILDDIR)/ia32/kernel/bin/kernel $(BUILDDIR)/efi-image/rainbow/kernel_ia32
	cp $(BUILDDIR)/x86_64/kernel/bin/kernel $(BUILDDIR)/efi-image/rainbow/kernel_x86_64
	mkdir -p $(BINDIR)
	dd if=/dev/zero of=$(BINDIR)/rainbow-uefi.img bs=1M count=33
	mkfs.vfat $(BINDIR)/rainbow-uefi.img -F32
	mcopy -s -i $(BINDIR)/rainbow-uefi.img $(BUILDDIR)/efi-image/* ::



###############################################################################
# Run targets
###############################################################################

.PHONY: run-bios-32
run-bios-32: bios-image
	qemu-system-i386 $(QEMUFLAGS) -cdrom $(BINDIR)/rainbow-bios.iso

.PHONY: run-bios-64
run-bios-64: bios-image
	qemu-system-x86_64 $(QEMUFLAGS) -cdrom $(BINDIR)/rainbow-bios.iso

.PHONY: run-efi-32
run-efi-32: efi-image
	qemu-system-i386 $(QEMUFLAGS) -bios $(ROOTDIR)/emulation/firmware/ovmf-ia32-r15214.fd $(BINDIR)/rainbow-uefi.img

.PHONY: run-efi-64
run-efi-64: efi-image
	qemu-system-x86_64 $(QEMUFLAGS) -bios $(ROOTDIR)/emulation/firmware/ovmf-x64-r15214.fd $(BINDIR)/rainbow-uefi.img

.PHONY: run-bochs
run-bochs: bios-image
	mkdir -p build/bochs
	cd build/bochs; bochs -f $(ROOTDIR)/emulation/bochs/config -q

.PHONY: run-bios
run-bios: run-bios-64

.PHONY: run-efi
run-efi: run-efi-64

.PHONY: run
run: run-bios
