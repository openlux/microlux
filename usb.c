#include <autovector.h>
#include <eputils.h>
#include <fx2macros.h>
#include <setupdat.h>

#include "microlux.h"
#include "usb.h"
#include "usb_cdc.h"

volatile bool usb_configured = false;
static volatile __bit dosud = FALSE;

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

    EP2CFG = bmVALID | bmTYPE1 | bmBUF1;
    SYNCDELAY;

    EP4CFG &= ~bmVALID;
    SYNCDELAY;

    EP6CFG = bmVALID | bmDIR | bmTYPE1 | bmBUF1;
    SYNCDELAY;

    EP8CFG &= ~bmVALID;
    SYNCDELAY;

    /* arm EP2 */
    EP2BCL = 0x80;
    SYNCDELAY;
    EP2BCL = 0x80;
    SYNCDELAY;
}

void usb_tick(void) {
    if (dosud) {
        dosud = FALSE;
        handle_setupdata();
    }
}

BOOL handle_get_descriptor() {
    return FALSE;
}

BOOL handle_vendorcommand(BYTE cmd) {
    return usb_cdc_handle_command(cmd);
}

BYTE handle_get_configuration(void) {
    return 1;
}

BOOL handle_set_configuration(BYTE cfg) {
    if (cfg == 1) {
        usb_configured = true;
        return TRUE;
    }
    return FALSE;
}

BOOL handle_get_interface(BYTE ifc, BYTE *alt_ifc) {
    if (ifc == 0) {
        *alt_ifc = 0;
        return TRUE;
    }
    return FALSE;
}

BOOL handle_set_interface(BYTE ifc, BYTE alt_ifc) {
    if (ifc == 0 && alt_ifc == 0) {
        /* reset EP1 in */
        EP1INCS |= bmEPBUSY;

        /* reset EP2 */
        RESETTOGGLE(0x02);

        EP2BCL = 0x80;
        SYNCDELAY;
        EP2BCL = 0x80;
        SYNCDELAY;

        RESETFIFO(0x02);

        /* reset EP6 */
        RESETTOGGLE(0x86);
        RESETFIFO(0x06);

        return TRUE;
    }
    return FALSE;
}

void handle_reset_ep(BYTE ep) {
    (void) ep; /* fx2lib doesn't call this function */
}

void sudav_isr(void) __interrupt SUDAV_ISR {
    dosud = TRUE;
    CLEAR_SUDAV();
}

void usbreset_isr(void) __interrupt USBRESET_ISR {
    handle_hispeed(FALSE);
    CLEAR_USBRESET();
}

void hispeed_isr(void) __interrupt HISPEED_ISR {
    handle_hispeed(TRUE);
    CLEAR_HISPEED();
}
