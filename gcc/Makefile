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

TARGET ?= i686-elf
PREFIX ?= $(HOME)/opt/cross

BUILDDIR ?= $(CURDIR)/build

GCC_VERSION ?= 6.1.0
BINUTILS_VERSION ?= 2.26


archive_dir := $(BUILDDIR)

binutils_name := binutils-$(BINUTILS_VERSION)
binutils_archive := $(archive_dir)/$(binutils_name).tar.bz2
binutils_src := $(BUILDDIR)/$(binutils_name)

gcc_name := gcc-$(GCC_VERSION)
gcc_archive := $(archive_dir)/$(gcc_name).tar.bz2
gcc_src := $(BUILDDIR)/$(gcc_name)

target_dir := $(BUILDDIR)/$(TARGET)


ifeq ($(CPU_COUNT),)
	OS:=$(shell uname -s)
	ifeq ($(OS),Linux)
		CPU_COUNT := $(shell grep -c ^processor /proc/cpuinfo)
	else ifeq ($(OS),Darwin)
		CPU_COUNT := $(shell system_profiler | awk '/Number Of CPUs/{print $4}{next;}')
	endif
endif


export PATH := $(PREFIX)/bin:$(PATH)



.PHONY: all
all: build-binutils build-gcc


.PHONY: install
install:
	$(MAKE) -s -C $(target_dir)/$(binutils_name) install
	$(MAKE) -s -C $(target_dir)/$(gcc_name) install-gcc
	$(MAKE) -s -C $(target_dir)/$(gcc_name) install-target-libgcc


.PHONY: clean
clean:
	$(RM) -r $(target_dir)


.PHONY: build-binutils
build-binutils: $(target_dir)/$(binutils_name)/Makefile
	$(MAKE) -j$(CPU_COUNT) -s -C $(dir $<)
	$(MAKE) -s -C $(dir $<) install

$(target_dir)/$(binutils_name)/Makefile: $(binutils_src)
	$(RM) -r $(dir $@)
	mkdir -p $(dir $@)
	cd $(dir $@); $</configure --target=$(TARGET) --prefix=$(PREFIX) --with-sysroot

$(binutils_src): $(binutils_archive)
	cd $(dir $@); bzip2 -dc $< | tar -xf -

$(binutils_archive):
	mkdir -p $(dir $@)
	cd $(dir $@); wget http://ftp.gnu.org/gnu/binutils/$(notdir $@)



.PHONY: build-gcc
build-gcc: $(target_dir)/$(gcc_name)/Makefile
	$(MAKE) -j$(CPU_COUNT) -s -C $(dir $<) all-gcc
	$(MAKE) -j$(CPU_COUNT) -s -C $(dir $<) all-target-libgcc CFLAGS_FOR_TARGET="-g -O2"
	$(MAKE) -s -C $(dir $<) install-gcc
	$(MAKE) -s -C $(dir $<) install-target-libgcc

$(target_dir)/$(gcc_name)/Makefile: $(gcc_src)
	$(RM) -r $(dir $@)
	mkdir -p $(dir $@)
	cd $(dir $@); $</configure --target=$(TARGET) --prefix=$(PREFIX) --without-headers --with-gnu-as --with-gnu-ld --enable-languages=c,c++

$(gcc_src): $(gcc_archive)
	cd $(dir $@); bzip2 -dc $< | tar -xf -

$(gcc_archive):
	mkdir -p $(dir $@)
	cd $(dir $@); wget http://ftp.gnu.org/gnu/gcc/$(gcc_name)/$(notdir $@)
