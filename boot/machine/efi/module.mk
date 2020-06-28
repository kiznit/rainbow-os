SOURCES := \
	efiboot.cpp \
	efidisplay.cpp \
	efifilesystem.cpp \
	jumpkernel.cpp \
	$(ARCH)/reloc.cpp \
	$(ARCH)/start.S

LDSCRIPT := $(ARCH)/efi.lds

CFLAGS += -fpic -fshort-wchar
CXXFLAGS += -fpic -fshort-wchar
LDFLAGS += -shared -Bsymbolic

ifeq ($(ARCH),x86_64)
	ARCH_FLAGS += -mno-red-zone
endif

TARGETS := boot.efi

%.efi: bootloader
	$(OBJCOPY) -j .text -j .rodata -j .data -j .dynamic -j .dynsym -j .rel.* -j .rela.* -j .reloc -j .init_array --target efi-app-$(ARCH) $< $@
