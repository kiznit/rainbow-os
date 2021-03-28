ifneq (,$(filter $(MACHINE),rpi rpi2 rpi3))
MODULES += machine/rpi
else
MODULES += machine/$(MACHINE)
endif
