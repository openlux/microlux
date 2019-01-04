#include <autovector.h>
#include <fx2macros.h>

#include "usb.h"
#include "usb_cdc.h"

void main(void) {
    /* set CPU clock frequency */
    SETCPUFREQ(CLK_48M);

    usb_init();

    /* enable interrupts */
    EA = 1;

    for (;;) {
        usb_tick();
        usb_cdc_tick();
    }
}
