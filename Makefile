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

# Find the root of the Rainbow OS source tree
ROOTDIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

# Where to find the Rainbow OS source code
SRCDIR ?= $(ROOTDIR)

# Where to find third party source code
THIRDPARTYDIR ?= $(ROOTDIR)/third_party

# Where to put intermediate build files
BUILDDIR ?= $(CURDIR)/build

# Where to put the build artifacts (final images)
BINDIR ?= $(CURDIR)/bin

# Default build targets host machine
TARGET_MACHINE ?= host

# Include configuration information
include mk/Makefile.common

export TARGET_MACHINE
export TARGET_ARCH



###############################################################################
# Top level targets
###############################################################################

.PHONY: all
all: rainbow-os-image


.PHONY: clean
clean:
	$(RM) -r $(BUILDDIR)
	$(RM) -r $(BINDIR)



###############################################################################
# Kernel
###############################################################################

.PHONY: kernel
kernel:
	$(MAKE) BUILDDIR=$(BUILDDIR)/$(TARGET_ARCH)/kernel -C $(SRCDIR)/kernel



###############################################################################
# Image
###############################################################################

.PHONY: rainbow-os-image
rainbow-os-image: kernel
