# Copyright (c) 2020, Thierry Tremblay
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

include $(TOPDIR)/src/mk/defaults.mk

BUILDDIR := $(TOPDIR)/build/$(ARCH)

MAKE := $(MAKE) -j$(CPU_COUNT) -rR


###############################################################################
#
# Machine
#
###############################################################################

ifeq ($(MAKECMDGOALS),run-bochs)
	MACHINE ?= bios
else
	MACHINE ?= efi
endif

ifeq ($(MACHINE),bios)
	BOOT_ARCH = ia32
else
	BOOT_ARCH = $(ARCH)
endif

IMAGE = $(MACHINE)_image


###############################################################################
#
# Targets
#
###############################################################################

MODULES := kernel go logger


.PHONY: all
all: boot $(MODULES)

.PHONY: clean
clean:
	$(RM) -r $(BUILDDIR)

.PHONE: test
test:
	mkdir -p $(TOPDIR)/build/test/boot
	$(MAKE) -C $(TOPDIR)/build/test/boot -f $(TOPDIR)/src/boot/test/Makefile test

.PHONY: coverage
coverage: test
	mkdir -p $(TOPDIR)/build/coverage/boot
	lcov --capture --no-external -d $(TOPDIR)/build/test/boot -b $(TOPDIR)/src/boot -o $(TOPDIR)/build/coverage/boot/coverage.info
	genhtml $(TOPDIR)/build/coverage/boot/coverage.info -o $(TOPDIR)/build/coverage/boot


.PHONY: boot
boot:
	mkdir -p $(BUILDDIR)/boot/$(MACHINE)
	KERNEL_ARCH=$(ARCH) ARCH=$(BOOT_ARCH) MACHINE=$(MACHINE) $(MAKE) -C $(BUILDDIR)/boot/$(MACHINE) -f $(TOPDIR)/src/boot/Makefile

.PHONY: kernel
kernel:
	mkdir -p $(BUILDDIR)/kernel
	$(MAKE) -C $(BUILDDIR)/kernel -f $(TOPDIR)/src/kernel/Makefile


.PHONY: libc
libc:
	mkdir -p $(BUILDDIR)/libs/libc
	$(MAKE) -C $(BUILDDIR)/libs/libc -f $(TOPDIR)/user/libs/libc/Makefile
	$(MAKE) -C $(BUILDDIR)/libs/libc -f $(TOPDIR)/user/libs/libc/Makefile install

.PHONY: libposix
libposix:
	mkdir -p $(BUILDDIR)/libs/posix
	$(MAKE) -C $(BUILDDIR)/libs/posix -f $(TOPDIR)/user/libs/posix/Makefile
	$(MAKE) -C $(BUILDDIR)/libs/posix -f $(TOPDIR)/user/libs/posix/Makefile install


.PHONY: go
go: libc libposix
	mkdir -p $(BUILDDIR)/services/go
	$(MAKE) -C $(BUILDDIR)/services/go -f $(TOPDIR)/user/services/go/Makefile

.PHONY: logger
logger: libc libposix
	mkdir -p $(BUILDDIR)/services/logger
	$(MAKE) -C $(BUILDDIR)/services/logger -f $(TOPDIR)/user/services/logger/Makefile


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
efi_image: boot $(MODULES)
	@ $(RM) -rf $(BUILDDIR)/image
	# bootloader
	mkdir -p $(BUILDDIR)/image/efi/rainbow
	cp $(BUILDDIR)/boot/efi/boot.efi $(BUILDDIR)/image/efi/rainbow/$(EFI_BOOTLOADER)
	# Fallback location for removal media (/efi/boot)
	mkdir -p $(BUILDDIR)/image/efi/boot
	cp $(BUILDDIR)/boot/efi/boot.efi $(BUILDDIR)/image/efi/boot/$(EFI_BOOTLOADER)
	# kernel
	cp $(BUILDDIR)/kernel/kernel $(BUILDDIR)/image/efi/rainbow/
	# go
	cp $(BUILDDIR)/services/go/go $(BUILDDIR)/image/efi/rainbow/
	# logger
	cp $(BUILDDIR)/services/logger/logger $(BUILDDIR)/image/efi/rainbow/
	# Build IMG
	dd if=/dev/zero of=$(BUILDDIR)/rainbow-efi.img bs=1M count=33
	mkfs.vfat $(BUILDDIR)/rainbow-efi.img -F32
	mcopy -s -i $(BUILDDIR)/rainbow-efi.img $(BUILDDIR)/image/* ::


###############################################################################
#
# BIOS Image
#
###############################################################################

.PHONY: bios_image
bios_image: boot $(MODULES)
	@ $(RM) -rf $(BUILDDIR)/image
	# Grub boot files
	mkdir -p $(BUILDDIR)/image/boot/grub
	cp $(TOPDIR)/src/boot/machine/bios/grub.cfg $(BUILDDIR)/image/boot/grub/grub.cfg
	# bootloader
	mkdir -p $(BUILDDIR)/image/boot/rainbow
	cp $(BUILDDIR)/boot/bios/boot $(BUILDDIR)/image/boot/rainbow/
	# kernel
	cp $(BUILDDIR)/kernel/kernel $(BUILDDIR)/image/boot/rainbow/
	# go
	cp $(BUILDDIR)/services/go/go $(BUILDDIR)/image/boot/rainbow/
	# logger
	cp $(BUILDDIR)/services/logger/logger $(BUILDDIR)/image/boot/rainbow/
	# Build ISO image
	grub-mkrescue -d /usr/lib/grub/i386-pc -o $(BUILDDIR)/rainbow-bios.img $(BUILDDIR)/image


###############################################################################
#
# Run image under QEMU
#
###############################################################################

QEMUFLAGS = \
	-monitor stdio \
	-m 8G \
	-net none \
	-drive format=raw,file=$(BUILDDIR)/rainbow-$(MACHINE).img

ifeq ($(ARCH),ia32)

	QEMU ?= qemu-system-i386
	QEMUFLAGS += \
		-accel kvm \
		-cpu Conroe -smp 4
	EFI_FIRMWARE ?= $(TOPDIR)/emulation/tianocore/ovmf-ia32-r15214.fd

else ifeq ($(ARCH),x86_64)

	QEMU ?= qemu-system-x86_64
	QEMUFLAGS += \
		-accel kvm \
		-smp 4
	EFI_FIRMWARE ?= $(TOPDIR)/emulation/tianocore/ovmf-x64-r15214.fd

endif


ifeq ($(MACHINE),efi)
	QEMUFLAGS += \
		-drive if=pflash,format=raw,file=$(EFI_FIRMWARE),readonly=on
endif


.PHONY: run-qemu
run-qemu: $(IMAGE)
	$(QEMU) $(QEMUFLAGS) #-d int,cpu_reset

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
	cd emulation/bochs; ARCH=$(ARCH) $(BOCHS) -f $(TOPDIR)/emulation/bochs/.bochsrc -q
