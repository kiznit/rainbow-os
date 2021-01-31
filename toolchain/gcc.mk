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

TARGET ?= i686-elf
PREFIX ?= $(HOME)/opt/cross
BUILDDIR ?= $(CURDIR)/build

BINUTILS_VERSION ?= 2.35
GCC_VERSION ?= 10.2.0
NEWLIB_VERSION ?= 4.1.0


archive_dir := $(BUILDDIR)

binutils_name := binutils-$(BINUTILS_VERSION)
binutils_archive := $(archive_dir)/$(binutils_name).tar.xz
binutils_src := $(BUILDDIR)/$(binutils_name)

gcc_name := gcc-$(GCC_VERSION)
gcc_archive := $(archive_dir)/$(gcc_name).tar.xz
gcc_src := $(BUILDDIR)/$(gcc_name)

newlib_name := newlib-$(NEWLIB_VERSION)
newlib_archive := $(archive_dir)/$(newlib_name).tar.gz
newlib_src := $(BUILDDIR)/$(newlib_name)

target_dir := $(BUILDDIR)/$(TARGET)


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
all: build-binutils build-gcc


.PHONY: clean
clean:
	$(RM) -r $(target_dir)


.PHONY: build-binutils
build-binutils: $(target_dir)/$(binutils_name)/Makefile
	$(MAKE) -j$(CPU_COUNT) -C $(dir $<)
	$(MAKE) -C $(target_dir)/$(binutils_name) install


.PHONY: build-gcc
build-gcc: $(target_dir)/$(gcc_name)/Makefile $(target_dir)/$(newlib_name)/Makefile
	# GCC (without libraries)
	$(MAKE) -j$(CPU_COUNT) -C $(target_dir)/$(gcc_name) all-gcc
	$(MAKE) -C $(target_dir)/$(gcc_name) install-gcc
	# libc (newlib)
	$(MAKE) -j$(CPU_COUNT) -C $(target_dir)/$(newlib_name) all
	$(MAKE) -C $(target_dir)/$(newlib_name) install
	# libgcc (has a dependency on newlib above)
	$(MAKE) -j$(CPU_COUNT) -C $(target_dir)/$(gcc_name) all-target-libgcc
	$(MAKE) -C $(target_dir)/$(gcc_name) install-target-libgcc
	# libstdc++ (has a dependency on newlib above)
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
		CFLAGS_FOR_TARGET="-D__rainbow__" \
		CXXFLAGS_FOR_TARGET="-D__rainbow__" \
		--enable-languages=c,c++ \
		--enable-libstdcxx \
		--enable-threads=posix \
		--enable-__cxa_atexit \
		--with-gnu-as \
		--with-gnu-ld \
		--with-newlib \
		--with-system-libunwind \
		--without-headers \
		$(GCC_CONFIGURE_FLAGS)

$(target_dir)/$(newlib_name)/Makefile: $(newlib_src)
	$(RM) -r $(dir $@)
	mkdir -p $(dir $@)
	cd $(dir $@); $</configure \
		--target=$(TARGET) \
		--prefix=$(PREFIX) \
		CFLAGS_FOR_TARGET="-g -O2 -D__rainbow__ -DGETREENT_PROVIDED -DHAVE_NANOSLEEP -DMALLOC_PROVIDED -DREENTRANT_SYSCALLS_PROVIDED" \
		--enable-newlib-global-atexit \
		--enable-newlib-global-stdio-streams \
		--enable-newlib-io-c99-formats \
		--enable-newlib-io-long-double \
		--enable-newlib-io-long-long \
		--enable-newlib-io-pos-args \
		--enable-newlib-reent-small \
		--enable-newlib-retargetable-locking \
		--disable-libgloss \
		$(NEWLIB_CONFIGURE_FLAGS)


$(binutils_src): $(binutils_archive)
	cd $(dir $@); tar -m -xf $<

$(gcc_src): $(gcc_archive)
	cd $(dir $@); tar -m -xf $<
	cd $@; ./contrib/download_prerequisites
	# Patch gcc and libgcc to add multilib support for x86_64
	patch -u $(gcc_src)/gcc/config.gcc -i patch/gcc/config.gcc.patch
	patch -u $(gcc_src)/libgcc/configure -i patch/libgcc/configure.patch
	cp patch/gcc/t-x86_64-kernel -T $(gcc_src)/gcc/config/i386/t-x86_64-kernel

$(newlib_src): $(newlib_archive)
	cd $(dir $@); tar -m -xf $<
	# Patch newlib
	patch -u $(newlib_src)/newlib/libc/include/sys/config.h -i patch/newlib/config.h.patch
	patch -u $(newlib_src)/newlib/libc/include/sys/features.h -i patch/newlib/features.h.patch
	patch -u $(newlib_src)/newlib/libc/stdio/local.h -i patch/newlib/local.h.patch
	# Rainbow headers
	cp -r rainbow/include $(newlib_src)/newlib/libc


$(binutils_archive):
	mkdir -p $(dir $@)
	cd $(dir $@); wget https://ftp.gnu.org/gnu/binutils/$(notdir $@)

$(gcc_archive):
	mkdir -p $(dir $@)
	cd $(dir $@); wget https://ftp.gnu.org/gnu/gcc/$(gcc_name)/$(notdir $@)

$(newlib_archive):
	mkdir -p $(dir $@)
	cd $(dir $@); wget ftp://sourceware.org/pub/newlib/$(notdir $@)
