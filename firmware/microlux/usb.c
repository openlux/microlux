#include <autovector.h>
#include <eputils.h>
#include <fx2macros.h>
#include <setupdat.h>

#include "ar0130.h"
#include "microlux.h"
#include "usb.h"
#include "usb_cdc.h"

volatile bool usb_configured = false;
static volatile __bit dosud = false;

void usb_init(void) {
    /* re-enumerate */
    RENUMERATE_UNCOND();

    /* enable USB interrupt autovectoring */
    USE_USB_INTS();

    /* enable USB interrupts */
    ENABLE_SUDAV();
    ENABLE_USBRESET();
    ENABLE_HISPEED();

    /* configure endpoints */
    EP1INCFG = bmVALID | bmTYPE1 | bmTYPE0;
    SYNCDELAY;

    EP1OUTCFG &= ~bmVALID;
    SYNCDELAY;

    EP2CFG = bmVALID | bmDIR | bmTYPE1;
    SYNCDELAY;

    EP4CFG = bmVALID | bmTYPE1;
    SYNCDELAY;

    EP6CFG &= ~bmVALID;
    SYNCDELAY;

    EP8CFG = bmVALID | bmDIR | bmTYPE1;
    SYNCDELAY;

    /* reset all FIFOs */
    RESETFIFOS();

    /* arm EP4 */
    OUTPKTEND = 0x84;
    SYNCDELAY;

    OUTPKTEND = 0x84;
    SYNCDELAY;
}

void usb_tick(void) {
    if (dosud) {
        dosud = false;
        handle_setupdata();
    }
}

bool handle_get_descriptor() {
    return false;
}

bool handle_vendorcommand(uint8_t cmd) {
    return ar0130_handle_command(cmd) || usb_cdc_handle_command(cmd);
}

uint8_t handle_get_configuration(void) {
    return 1;
}

bool handle_set_configuration(uint8_t cfg) {
    if (cfg == 1) {
        usb_configured = true;
        return true;
    }
    return false;
}

bool handle_get_interface(uint8_t ifc, uint8_t *alt_ifc) {
    if (ifc == 0) {
        *alt_ifc = 0;
        return true;
    }
    return false;
}

bool handle_set_interface(uint8_t ifc, uint8_t alt_ifc) {
    if (ifc == 0 && alt_ifc == 0) {
        /* reset EP1 in */
        EP1INCS |= bmEPBUSY;

        /* reset EP2 */
        RESETTOGGLE(0x82);

        FIFORESET = 0x80;
        SYNCDELAY;
        EP2FIFOCFG &= ~bmAUTOIN;
        SYNCDELAY;
        FIFORESET = 0x02;
        SYNCDELAY;
        EP2FIFOCFG |= bmAUTOIN;
        SYNCDELAY;
        FIFORESET = 0x00;
        SYNCDELAY;

        /* reset EP4 */
        RESETTOGGLE(0x04);

        OUTPKTEND = 0x84;
        SYNCDELAY;

        OUTPKTEND = 0x84;
        SYNCDELAY;

        RESETFIFO(0x04);

        /* reset EP8 */
        RESETTOGGLE(0x88);
        RESETFIFO(0x08);

        return true;
    }
    return false;
}

void handle_reset_ep(uint8_t ep) {
    (void) ep; /* fx2lib doesn't call this function */
}

void sudav_isr(void) __interrupt SUDAV_ISR {
    dosud = true;
    CLEAR_SUDAV();
}

void usbreset_isr(void) __interrupt USBRESET_ISR {
    handle_hispeed(false);
    CLEAR_USBRESET();
}

void hispeed_isr(void) __interrupt HISPEED_ISR {
    handle_hispeed(true);
    CLEAR_HISPEED();
}
