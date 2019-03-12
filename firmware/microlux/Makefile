FX2LIB=../fx2lib

SOURCES=ar0130.c i2c.c microlux.c uitoa.c usb.c usb_ar0130.c usb_cdc.c usb_fifo.c
A51_SOURCES=dscr.a51
BASENAME=microlux
BUILDDIR=build

.PHONY: hex

hex: $(BUILDDIR)/$(BASENAME).hex

$(BUILDDIR)/$(BASENAME).hex: $(BUILDDIR)/$(BASENAME).ihx
	cp $< $@

include $(FX2LIB)/lib/fx2.mk
