#include <autovector.h>
#include <fx2macros.h>

#include "ar0130.h"
#include "i2c.h"
#include "usb.h"
#include "usb_cdc.h"
#include "usb_fifo.h"

void main(void) {
    /* enable revision-specific features */
    REVCTL = bmNOAUTOARM | bmSKIPCOMMIT;

    /* set CPU clock frequency to 48 MHz and enable CLKOUT */
    CPUCS = bmCLKSPD1 | bmCLKOE;

    /* enable internal 48 MHz IFCLK (we switch to the external IFCLK after
     * initializing the AR0130 chip) */
    IFCONFIG = bmIFCLKSRC | bm3048MHZ;

    /* start external IFCLK */
    i2c_init();
    ar0130_init();

    /* switch to external 48 MHz IFCLK */
    IFCONFIG = 0;

    usb_init();
    usb_fifo_init();

    /* enable interrupts */
    EA = 1;

    for (;;) {
        usb_tick();
        usb_cdc_tick();
    }
}