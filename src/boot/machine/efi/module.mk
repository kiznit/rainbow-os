SOURCES := \
	efiboot.cpp \
	efidisplay.cpp \
	efifilesystem.cpp \
	jumpkernel.cpp \
	malloc.cpp \
	$(ARCH)/reloc.cpp \
	$(ARCH)/start.S

LDSCRIPT := $(ARCH)/efi.lds

CFLAGS += -fpic
CXXFLAGS += -fpic
LDFLAGS += -shared -Bsymbolic -Wl,-pie

ifeq ($(ARCH),x86_64)
	ARCH_FLAGS += -mno-red-zone
endif

TARGETS += boot.efi

%.efi: boot
	$(OBJCOPY) -j .text -j .rodata -j .data -j .dynamic -j .dynsym -j .rel.* -j .rela.* -j .reloc --target efi-app-$(ARCH) $< $@
