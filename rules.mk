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


###############################################################################
#
# Usual suspects
#
###############################################################################

%.c.o %.c.d: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -MMD -c $< -o $(@:%.d=%.o)

%.cpp.o %.cpp.d: %.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -MMD -c $< -o $(@:%.d=%.o)

%.S.o %.S.d: %.S
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) $(CPPFLAGS) -MMD -c $< -o $(@:%.d=%.o)



###############################################################################
#
# Modules
#
###############################################################################

# call $(add-module,MODULENAME,MODULEDIR)
define add-module
$(info Adding module '$1' at '$2')
previous_sources := $$(SOURCES)
SOURCES :=
include $2/module.mk
SOURCES := $$(previous_sources) $$(SOURCES:%=$1/%)
endef


# List of loaded modules (to prevent multiple includes)
loaded_modules :=

# call $(find-module,MODULE)
define find-module
ifeq ($(filter $1,$(loaded_modules)),)
match := $$(dir $$(realpath $$(word 1, $$(foreach PATH, $(MODULEPATH), $$(wildcard $$(PATH)/$1/module.mk)))))
ifneq ($$(match),)
    module_name := $$(shell realpath --relative-to $$(TOPDIR) $$(match))
    $$(eval $$(call add-module,$$(module_name),$$(match)))
    loaded_modules += $1
else
    $$(error Could not find module '$1' in search path '$(MODULEPATH)')
endif
endif
endef


# call $(load-modules,MODULES)
define load-modules
$(foreach MODULE, $1, $(eval $(call find-module,$(MODULE))))
endef
