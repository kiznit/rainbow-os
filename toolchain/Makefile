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


.PHONY: all
all:
	@echo "Please specify a target"


# 32-bit x86
.PHONY i686-rainbow-elf:
i686-rainbow-elf:
	$(MAKE) TARGET=$@ BINUTILS_CONFIGURE_FLAGS="--enable-targets=all" GCC_CONFIGURE_FLAGS="--with-arch=i686" -f gcc.mk


# 64-bit x86
.PHONY x86_64-rainbow-elf:
x86_64-rainbow-elf:
	$(MAKE) TARGET=$@ BINUTILS_CONFIGURE_FLAGS="--enable-targets=all" -f gcc.mk


# 32-bit ARM
.PHONY arm-rainbow-eabi:
arm-rainbow-eabi:
	$(MAKE) TARGET=$@ GCC_CONFIGURE_FLAGS="--with-multilib-list=aprofile" -f gcc.mk


# 64-bit ARM
.PHONY aarch64-rainbow-elf:
aarch64-rainbow-elf:
	$(MAKE) TARGET=$@ GCC_CONFIGURE_FLAGS="--enable-fix-cortex-a53-835769 --enable-fix-cortex-a53-843419" -f gcc.mk
