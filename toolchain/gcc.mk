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

TARGET ?= x86_64-rainbow-elf
PREFIX ?= $(HOME)/opt/cross
BUILDDIR ?= $(CURDIR)/build

BINUTILS_VERSION ?= 2.35
GCC_VERSION ?= 10.2.0
NEWLIB_VERSION ?= 4.1.0

archive_dir = $(BUILDDIR)/packages
src_dir = $(BUILDDIR)/src

binutils_name = binutils-$(BINUTILS_VERSION)
binutils_archive = $(archive_dir)/$(binutils_name).tar.xz
binutils_src = $(src_dir)/$(binutils_name)

gcc_name = gcc-$(GCC_VERSION)
gcc_archive = $(archive_dir)/$(gcc_name).tar.xz
gcc_src = $(src_dir)/$(gcc_name)

newlib_name = newlib-$(NEWLIB_VERSION)
newlib_archive = $(archive_dir)/$(newlib_name).tar.gz
newlib_src = $(src_dir)/$(newlib_name)

target_dir = $(BUILDDIR)/out/$(TARGET)


ifeq ($(CPU_COUNT),)
	OS:=$(shell uname -s)
	ifeq ($(OS),Linux)
		CPU_COUNT := $(shell grep -c ^processor /proc/cpuinfo)
	else ifeq ($(OS),Darwin)
		CPU_COUNT := $(shell system_profiler | awk '/Number Of CPUs/{print $4}{next;}')
	else ifneq ($(NUMBER_OF_PROCESSORS),)
		CPU_COUNT := $(NUMBER_OF_PROCESSORS)
	else
		CPU_COUNT := 1
	endif
endif


export PATH := $(PREFIX)/bin:$(PATH)



.PHONY: all
all: libstdc++


.PHONY: clean
clean:
	$(RM) -r $(target_dir)


.PHONY: binutils
binutils: $(target_dir)/$(binutils_name)/Makefile
	$(MAKE) -j$(CPU_COUNT) -C $(dir $<)
	$(MAKE) -C $(target_dir)/$(binutils_name) install

.PHONY: gcc
gcc: binutils $(target_dir)/$(gcc_name)/Makefile
	$(MAKE) -j$(CPU_COUNT) -C $(target_dir)/$(gcc_name) all-gcc
	$(MAKE) -C $(target_dir)/$(gcc_name) install-gcc

.PHONY: newlib
newlib: gcc $(target_dir)/$(newlib_name)/Makefile
	$(MAKE) -j$(CPU_COUNT) -C $(target_dir)/$(newlib_name) all
	$(MAKE) -C $(target_dir)/$(newlib_name) install

.PHONY: libgcc
libgcc: newlib
	$(MAKE) -j$(CPU_COUNT) -C $(target_dir)/$(gcc_name) all-target-libgcc
	$(MAKE) -C $(target_dir)/$(gcc_name) install-target-libgcc

.PHONY: libstdc++
libstdc++: libgcc
	$(MAKE) -C $(target_dir)/$(gcc_name) all-target-libstdc++-v3
	$(MAKE) -C $(target_dir)/$(gcc_name) install-target-libstdc++-v3


$(target_dir)/$(binutils_name)/Makefile: $(binutils_src)
	$(RM) -r $(dir $@)
	mkdir -p $(dir $@)
	cd $(dir $@); $</configure \
		--target=$(TARGET) \
		--prefix=$(PREFIX) \
		$(BINUTILS_CONFIGURE_FLAGS)

$(target_dir)/$(gcc_name)/Makefile: $(gcc_src)
	$(RM) -r $(dir $@)
	mkdir -p $(dir $@)
	cd $(dir $@); $</configure \
		--target=$(TARGET) \
		--prefix=$(PREFIX) \
		--enable-initfini-array \
		--enable-languages=c,c++ \
		--enable-libstdcxx \
		--enable-threads=posix \
		--enable-__cxa_atexit \
		--with-newlib \
		--without-headers \
		$(GCC_CONFIGURE_FLAGS)

$(target_dir)/$(newlib_name)/Makefile: $(newlib_src) gcc
	$(RM) -r $(dir $@)
	mkdir -p $(dir $@)
	cd $(dir $@); $</configure \
		--target=$(TARGET) \
		--prefix=$(PREFIX) \
		CFLAGS_FOR_TARGET="-g -O2 -DGETREENT_PROVIDED -DHAVE_NANOSLEEP -DMALLOC_PROVIDED -DMISSING_SYSCALL_NAMES -DREENTRANT_SYSCALLS_PROVIDED" \
		--enable-newlib-global-atexit \
		--enable-newlib-global-stdio-streams \
		--enable-newlib-io-c99-formats \
		--enable-newlib-io-long-double \
		--enable-newlib-io-long-long \
		--enable-newlib-io-pos-args \
		--enable-newlib-retargetable-locking \
		--disable-libgloss \
		$(NEWLIB_CONFIGURE_FLAGS)


$(binutils_src): $(binutils_archive)
	mkdir -p $(dir $@)
	cd $(dir $@); tar -m -xf $<

$(gcc_src): $(gcc_archive)
	mkdir -p $(dir $@)
	cd $(dir $@); tar -m -xf $<
	cd $@; ./contrib/download_prerequisites
	# Patch gcc and libgcc (rainbow OS config + multilib support for x86_64)
	patch -u $(gcc_src)/gcc/config.gcc -i patch/gcc/config.gcc.patch
	patch -u $(gcc_src)/libgcc/configure -i patch/libgcc/configure.patch
	cp patch/gcc/rainbow.h -T $(gcc_src)/gcc/config/rainbow.h
	cp patch/gcc/t-i386-rainbow -T $(gcc_src)/gcc/config/i386/t-i386-rainbow
	cp patch/gcc/t-x86_64-rainbow -T $(gcc_src)/gcc/config/i386/t-x86_64-rainbow
	# Patch libstdc++ to work on x86_64 with disabled floats
	patch -u $(gcc_src)/libstdc++-v3/include/bits/std_abs.h -i patch/libstdc++/std_abs.h.patch
	patch -u $(gcc_src)/libstdc++-v3/include/bits/hashtable_policy.h -i patch/libstdc++/hashtable_policy.h.patch
	patch -u $(gcc_src)/libstdc++-v3/include/std/limits -i patch/libstdc++/limits.patch

$(newlib_src): $(newlib_archive)
	mkdir -p $(dir $@)
	cd $(dir $@); tar -m -xf $<
	# Patch newlib
	patch -u $(newlib_src)/newlib/libc/include/sys/config.h -i patch/newlib/config.h.patch
	patch -u $(newlib_src)/newlib/libc/include/sys/features.h -i patch/newlib/features.h.patch
	patch -u $(newlib_src)/newlib/libc/stdio/local.h -i patch/newlib/local.h.patch
	# Rainbow headers
	# TODO: using userspace headers for the kernel? are we happy with this?
	cp -r ../user/include $(newlib_src)/newlib/libc


$(binutils_archive):
	mkdir -p $(dir $@)
	cd $(dir $@); wget https://ftp.gnu.org/gnu/binutils/$(notdir $@)

$(gcc_archive):
	mkdir -p $(dir $@)
	cd $(dir $@); wget https://ftp.gnu.org/gnu/gcc/$(gcc_name)/$(notdir $@)

$(newlib_archive):
	mkdir -p $(dir $@)
	cd $(dir $@); wget ftp://sourceware.org/pub/newlib/$(notdir $@)
