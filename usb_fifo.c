#include <eputils.h>
#include <fx2macros.h>
#include <stdint.h>

#include "microlux.h"
#include "usb_fifo.h"

/* FIFOINPOLAR is missing from fx2regs.h */
static __xdata __at 0xE609 volatile uint8_t FIFOINPOLAR;
#define bmSLOE bmBIT4
#define bmSLWR bmBIT2

void usb_fifo_init(void) {
    /* enable synchronous slave FIFO with external IFCLK */
    IFCONFIG |= bmIFCFG1 | bmIFCFG0;
    SYNCDELAY;

    /* swap SLOE and SLWR polarity */
    FIFOINPOLAR = bmSLOE | bmSLWR;
    SYNCDELAY;

    /* bypass CPU on EP2 IN, enable ZLPs and set FIFO width to 16 bits */
    EP2FIFOCFG = bmAUTOIN | bmZEROLENIN | bmWORDWIDE;
    SYNCDELAY;

    /* set auto-read length to 512 bytes (TODO: set to 64 bytes in full speed
     * mode) */
    EP2AUTOINLENH = 0x02;
    SYNCDELAY;

    EP2AUTOINLENL = 0x00;
    SYNCDELAY;
}

void usb_fifo_flush(void) {
    INPKTEND = 0x02;
    SYNCDELAY;
}
