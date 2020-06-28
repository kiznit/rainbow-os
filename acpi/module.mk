ACPICA_DIRS = dispatcher events executer hardware namespace parser resources tables utilities
ACPICA_GLOB = $(foreach dir,$(ACPICA_DIRS),$(shell find $(ACPICA_PATH)/source/components/$(dir) -name '*.c'))
ACPICA_SOURCES = $(filter-out acpica/source/components/resources/rsdump.c,$(subst $(ACPICA_PATH),acpica,$(ACPICA_GLOB)))

SOURCES := \
	acpi.c \
	crt.cpp \
	main.c \
	$(ACPICA_SOURCES)
