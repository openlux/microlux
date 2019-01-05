#include <autovector.h>
#include <fx2macros.h>

#include "i2c.h"
#include "usb.h"
#include "usb_cdc.h"

void main(void) {
    /* set CPU clock frequency to 48 MHz and enable CLKOUT */
    CPUCS = bmCLKSPD1 | bmCLKOE;

    i2c_init();
    usb_init();

    /* enable interrupts */
    EA = 1;

    for (;;) {
        usb_tick();
        usb_cdc_tick();
    }
}
