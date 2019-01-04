#include <autovector.h>
#include <eputils.h>
#include <fx2macros.h>
#include <setupdat.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define SYNCDELAY SYNCDELAY4

#define CDC_SET_LINE_CODING        0x20
#define CDC_GET_LINE_CODING        0x21
#define CDC_SET_CONTROL_LINE_STATE 0x22

struct cdc_line_coding {
    uint32_t baud;
    uint8_t stop_bits;
    uint8_t parity;
    uint8_t data_bits;
};

static volatile __bit dosud = FALSE;
static volatile __bit configured = FALSE;
static struct cdc_line_coding cdc_line_coding;
static uint16_t cdc_line_state;

static void cdc_puts(const char *str) {
    /* wait for free space in EP6 */
    while (EP2468STAT & bmEP6FULL);

    /* write str to the EP6 FIFO */
    strcpy(EP6FIFOBUF, str);

    /* send the data to the host */
    size_t len = strlen(str);
    EP6BCH = MSB(len);
    SYNCDELAY;
    EP6BCL = LSB(len);
    SYNCDELAY;
}

void main(void) {
    /* re-enumerate */
    RENUMERATE_UNCOND();

    /* set CPU clock frequency */
    SETCPUFREQ(CLK_48M);

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

    /* enable interrupts */
    EA = 1;

    for (;;) {
        if (dosud) {
            dosud = FALSE;
            handle_setupdata();
        }

        if (configured) {
            cdc_puts("Hello, world!\r\n");
        }
    }
}

BOOL handle_get_descriptor() {
    return FALSE;
}

BOOL handle_vendorcommand(BYTE cmd) {
    size_t len = sizeof(cdc_line_coding);
    switch (cmd) {
        case CDC_SET_LINE_CODING:
            while (EP0BCL < len);
            memcpy(&cdc_line_coding, EP0BUF, len);
            EP0BCL = 0;
            return TRUE;
        case CDC_GET_LINE_CODING:
            SUDPTRCTL &= ~bmSDPAUTO;
            EP0BCH = MSB(len);
            EP0BCL = LSB(len);
            SUDPTRH = MSB(&cdc_line_coding);
            SUDPTRL = MSB(&cdc_line_coding);
            SUDPTRCTL |= bmSDPAUTO;
            return TRUE;
        case CDC_SET_CONTROL_LINE_STATE:
            cdc_line_state = SETUP_VALUE();
            return TRUE;
    }
    return FALSE;
}

BYTE handle_get_configuration(void) {
    return 1;
}

BOOL handle_set_configuration(BYTE cfg) {
    if (cfg == 1) {
        configured = TRUE;
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
