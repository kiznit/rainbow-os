ifneq (,$(filter $(MACHINE),raspi raspi2 raspi3))
MODULES += machine/raspi
else
MODULES += machine/$(MACHINE)
endif
