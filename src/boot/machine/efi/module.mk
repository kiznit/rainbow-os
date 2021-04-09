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
ASFLAGS += -fpic
LDFLAGS += -shared -Bsymbolic -Wl,-pie

ifeq ($(ARCH),x86_64)
	ARCH_FLAGS += -mno-red-zone
endif

TARGETS += boot.efi

ifeq ($(ARCH),aarch64)
	FORMAT = -O binary
else
	FORMAT = --target efi-app-$(ARCH)
endif

ifeq ($(ARCH),ia32)
	OBJDUMP_ARCH = i386
else ifeq ($(ARCH),x86_64)
	OBJDUMP_ARCH = i386:x86-64
else
	OBJDUMP_ARCH = $(ARCH)
endif

%.efi: boot
	$(OBJCOPY) -j .text -j .rodata -j .data -j .dynamic -j .dynsym -j .rel.* -j .rela.* -j .reloc $(FORMAT) $< $@
	$(OBJDUMP) -D -b binary -m$(OBJDUMP_ARCH) $@ > $@.disassembly
