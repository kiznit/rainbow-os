ACPICA_IGNORE = \
	acpica/source/components/resources/rsdump.c \
	acpica/source/components/utilities/utprint.c

#TODO: I am pretty sure I am compiling and linking things I don't need here...
ACPICA_DIRS = dispatcher events executer hardware namespace parser resources tables utilities
ACPICA_GLOB = $(foreach dir,$(ACPICA_DIRS),$(shell find $(ACPICA_PATH)/source/components/$(dir) -name '*.c'))
ACPICA_SOURCES = $(filter-out $(ACPICA_IGNORE),$(subst $(ACPICA_PATH),acpica,$(ACPICA_GLOB)))

SOURCES := \
	acpi.c \
	main.c \
	$(ACPICA_SOURCES)
