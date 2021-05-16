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

MAKE := $(MAKE) -j$(CPU_COUNT) -rR


# Set default build directory
BUILDDIR ?= $(TOPDIR)/build

# Build directory for arch-specific artifacts (kernel, user space)
ARCHDIR = $(BUILDDIR)/arch/$(ARCH)

# Build directory for machine-specific artifacts (bootloader, emulation support)
MACHINEDIR = $(BUILDDIR)/$(MACHINE)-$(ARCH)

# Build directory for images
IMAGEDIR = $(MACHINEDIR)


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
	BOOT_MACHINE = bios
else
	BOOT_ARCH = $(ARCH)
	BOOT_MACHINE = efi
endif

IMAGE = $(BOOT_MACHINE)_image


ifeq ($(MACHINE),raspi3)
ifeq ($(BOOT_MACHINE),efi)
IMAGE_EXTRA_COMMANDS = cp -R $(TOPDIR)/src/third_party/firmware/raspi3_uefi_aarch64/* $(IMAGEDIR)/image
endif
endif


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
	mkdir -p $(MACHINEDIR)/boot
	$(MAKE) KERNEL_ARCH=$(ARCH) ARCH=$(BOOT_ARCH) MACHINE=$(BOOT_MACHINE) -C $(MACHINEDIR)/boot -f $(TOPDIR)/src/boot/Makefile

.PHONY: kernel
kernel:
	mkdir -p $(ARCHDIR)/kernel
	$(MAKE) -C $(ARCHDIR)/kernel -f $(TOPDIR)/src/kernel/Makefile


.PHONY: libc
libc:
	mkdir -p $(ARCHDIR)/libs/libc
	$(MAKE) -C $(ARCHDIR)/libs/libc -f $(TOPDIR)/user/libs/libc/Makefile
	$(MAKE) -C $(ARCHDIR)/libs/libc -f $(TOPDIR)/user/libs/libc/Makefile install

.PHONY: libposix
libposix:
	mkdir -p $(ARCHDIR)/libs/posix
	$(MAKE) -C $(ARCHDIR)/libs/posix -f $(TOPDIR)/user/libs/posix/Makefile
	$(MAKE) -C $(ARCHDIR)/libs/posix -f $(TOPDIR)/user/libs/posix/Makefile install


.PHONY: go
go: libc libposix
	mkdir -p $(ARCHDIR)/services/go
	$(MAKE) -C $(ARCHDIR)/services/go -f $(TOPDIR)/user/services/go/Makefile

.PHONY: logger
logger: libc libposix
	mkdir -p $(ARCHDIR)/services/logger
	$(MAKE) -C $(ARCHDIR)/services/logger -f $(TOPDIR)/user/services/logger/Makefile


.PHONY: image
image: $(IMAGE)



###############################################################################
#
# EFI Image
#
###############################################################################

ifeq ($(ARCH),ia32)
EFI_BOOTLOADER = bootia32.efi
else ifeq ($(ARCH),x86_64)
EFI_BOOTLOADER = bootx64.efi
else ifeq ($(ARCH),aarch64)
EFI_BOOTLOADER = bootaa64.efi
endif

.PHONY: efi_image
efi_image: boot $(MODULES)
	@ $(RM) -rf $(IMAGEDIR)/image
	# bootloader
	mkdir -p $(IMAGEDIR)/image/efi/rainbow
	cp $(MACHINEDIR)/boot/boot.efi $(IMAGEDIR)/image/efi/rainbow/$(EFI_BOOTLOADER)
	# Fallback location for removal media (/efi/boot)
	mkdir -p $(IMAGEDIR)/image/efi/boot
	cp $(MACHINEDIR)/boot/boot.efi $(IMAGEDIR)/image/efi/boot/$(EFI_BOOTLOADER)
	# kernel
	cp $(ARCHDIR)/kernel/kernel $(IMAGEDIR)/image/efi/rainbow/
	# go
	cp $(ARCHDIR)/services/go/go $(IMAGEDIR)/image/efi/rainbow/
	# logger
	cp $(ARCHDIR)/services/logger/logger $(IMAGEDIR)/image/efi/rainbow/
	# Extra commands
	$(IMAGE_EXTRA_COMMANDS)
	# Build IMG
	dd if=/dev/zero of=$(IMAGEDIR)/rainbow-efi.img bs=1M count=33
	mkfs.vfat $(IMAGEDIR)/rainbow-efi.img -F32
	mcopy -s -i $(IMAGEDIR)/rainbow-efi.img $(IMAGEDIR)/image/* ::


###############################################################################
#
# BIOS Image
#
###############################################################################

.PHONY: bios_image
bios_image: boot $(MODULES)
	@ $(RM) -rf $(IMAGEDIR)/image
	# Grub boot files
	mkdir -p $(MACHINEDIR)/image/boot/grub
	cp $(TOPDIR)/src/boot/machine/bios/grub.cfg $(IMAGEDIR)/image/boot/grub/grub.cfg
	# bootloader
	mkdir -p $(MACHINEDIR)/image/boot/rainbow
	cp $(MACHINEDIR)/boot/boot $(IMAGEDIR)/image/boot/rainbow/
	# kernel
	cp $(ARCHDIR)/kernel/kernel $(IMAGEDIR)/image/boot/rainbow/
	# go
	cp $(ARCHDIR)/services/go/go $(IMAGEDIR)/image/boot/rainbow/
	# logger
	cp $(ARCHDIR)/services/logger/logger $(IMAGEDIR)/image/boot/rainbow/
	# Build ISO image
	grub-mkrescue -d /usr/lib/grub/i386-pc -o $(IMAGEDIR)/rainbow-bios.img $(IMAGEDIR)/image


###############################################################################
#
# EFI Emulation Firmware
#
###############################################################################

# Copy the emulation firmware file for ia32 as it includes NVRAM
$(MACHINEDIR)/emulation/ovmf-ia32-pure-efi.fd: emulation/tianocore/ovmf-ia32-pure-efi.fd
	mkdir -p $(MACHINEDIR)/emulation
	cp $< $@

# Copy the emulation firmware file for ia32 as it includes NVRAM
$(MACHINEDIR)/emulation/ovmf-x86_64-pure-efi.fd: emulation/tianocore/ovmf-x86_64-pure-efi.fd
	mkdir -p $(MACHINEDIR)/emulation
	cp $< $@

# The aarch64 emulation firmware needs to be padded to 64m (don't know why, might have to do with using "-machine virt")
$(MACHINEDIR)/emulation/efi.img: emulation/tianocore/omvf-aarch64.fd
	mkdir -p $(MACHINEDIR)/emulation
	truncate -s 64m $@
	dd if=$< of=$@ conv=notrunc

# The aarch64 emulation nvram also needs to be padded to 64m (don't know why, might have to do with using "-machine virt")
$(MACHINEDIR)/emulation/nvram.img:
	mkdir -p $(MACHINEDIR)/emulation
	truncate -s 64m $@


###############################################################################
#
# Run image under QEMU
#
###############################################################################

QEMU_FLAGS = \
	-monitor stdio \
	-m 8G \
	-net none \
	-drive format=raw,file=$(IMAGEDIR)/rainbow-$(MACHINE).img

ifeq ($(ARCH),ia32)
	QEMU ?= qemu-system-i386
	QEMU_FLAGS += \
		-accel kvm \
		-cpu Conroe -smp 4
	# This firmware file includes both code and NVARAM
	EFI_DEPS = $(MACHINEDIR)/emulation/ovmf-ia32-pure-efi.fd
	EFI_FIRMWARE = -drive if=pflash,format=raw,file=$(EFI_DEPS)

else ifeq ($(ARCH),x86_64)
	QEMU ?= qemu-system-x86_64
	QEMU_FLAGS += \
		-accel kvm \
		-smp 4
	# This firmware file includes both code and NVARAM
	EFI_DEPS = $(MACHINEDIR)/emulation/ovmf-x86_64-pure-efi.fd
	EFI_FIRMWARE = -drive if=pflash,format=raw,file=$(EFI_DEPS)

else ifeq ($(ARCH),aarch64)
	QEMU ?= qemu-system-aarch64
	QEMU_FLAGS += \
		-machine virt \
		-cpu cortex-a53 \
		-device virtio-gpu-pci
	EFI_DEPS = $(MACHINEDIR)/emulation/efi.img $(MACHINEDIR)/emulation/nvram.img
	EFI_FIRMWARE = \
		-drive if=pflash,format=raw,file=$(MACHINEDIR)/emulation/efi.img,readonly \
		-drive if=pflash,format=raw,file=$(MACHINEDIR)/emulation/nvram.img
endif

ifeq ($(MACHINE),efi)
	QEMU_FLAGS += $(EFI_FIRMWARE)
	QEMU_DEPS += $(EFI_DEPS)
endif


.PHONY: run-qemu
run-qemu: $(IMAGE) $(QEMU_DEPS)
	$(QEMU) $(QEMU_FLAGS) #-d int,cpu_reset

.PHONY: debug
debug: $(IMAGE)
	$(QEMU) $(QEMU_FLAGS) -s -S -debugcon file:debug.log -global isa-debugcon.iobase=0x402

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
