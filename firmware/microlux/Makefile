FX2LIB=../fx2lib

SOURCES=ar0130.c i2c.c microlux.c uitoa.c usb.c usb_ar0130.c usb_cdc.c usb_fifo.c
A51_SOURCES=dscr.a51
BASENAME=microlux
BUILDDIR=build

.PHONY: copy
copy: ihx
	cp $(BUILDDIR)/$(BASENAME).ihx $(BUILDDIR)/$(BASENAME).hex

include $(FX2LIB)/lib/fx2.mk

