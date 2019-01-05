#include <autovector.h>
#include <fx2macros.h>

#include "i2c.h"
#include "usb.h"
#include "usb_cdc.h"

void main(void) {
    /* enable revision-specific features */
    REVCTL = bmNOAUTOARM | bmSKIPCOMMIT;

    /* set CPU clock frequency to 48 MHz and enable CLKOUT */
    CPUCS = bmCLKSPD1 | bmCLKOE;

    /* enable internal 48 MHz IFCLK (we switch to the external IFCLK after
     * initializing the AR0130 chip) */
    IFCONFIG = bmIFCLKSRC | bm3048MHZ;

    i2c_init();
    usb_init();

    /* enable interrupts */
    EA = 1;

    for (;;) {
        usb_tick();
        usb_cdc_tick();
    }
}
