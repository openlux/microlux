#include <autovector.h>
#include <eputils.h>
#include <fx2macros.h>
#include <setupdat.h>

#define SYNCDELAY SYNCDELAY4

static volatile __bit dosud = FALSE;

void main(void) {
    RENUMERATE_UNCOND();

    SETCPUFREQ(CLK_48M);

    USE_USB_INTS();

    ENABLE_SUDAV();
    ENABLE_USBRESET();
    ENABLE_HISPEED();

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

    EA = 1;

    for (;;) {
        if (dosud) {
            dosud = FALSE;
            handle_setupdata();
        }
    }
}

BOOL handle_get_descriptor() {
    return FALSE;
}

BOOL handle_vendorcommand(BYTE cmd) {
    return FALSE;
}

BYTE handle_get_configuration(void) {
    return 1;
}

BOOL handle_set_configuration(BYTE cfg) {
    return cfg == 1;
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
        RESETFIFO(0x02);

        /* reset EP6 */
        RESETTOGGLE(0x86);
        RESETFIFO(0x06);

        return TRUE;
    }
    return FALSE;
}

void handle_reset_ep(BYTE ep) {

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
