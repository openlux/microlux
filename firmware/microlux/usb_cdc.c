#include <fx2macros.h>
#include <setupdat.h>
#include <stddef.h>
#include <string.h>

#include "microlux.h"
#include "usb_cdc.h"

#define CDC_SET_LINE_CODING        0x20
#define CDC_GET_LINE_CODING        0x21
#define CDC_SET_CONTROL_LINE_STATE 0x22

struct cdc_line_coding {
    uint32_t baud;
    uint8_t stop_bits;
    uint8_t parity;
    uint8_t data_bits;
};

static struct cdc_line_coding cdc_line_coding;
static uint16_t cdc_line_state;

bool usb_cdc_handle_command(uint8_t cmd) {
    size_t len = sizeof(cdc_line_coding);
    switch (cmd) {
        case CDC_SET_LINE_CODING:
            while (EP0BCL < len);
            memcpy(&cdc_line_coding, EP0BUF, len);
            EP0BCL = 0;
            return true;
        case CDC_GET_LINE_CODING:
            SUDPTRCTL &= ~bmSDPAUTO;
            EP0BCH = MSB(len);
            EP0BCL = LSB(len);
            SUDPTRH = MSB(&cdc_line_coding);
            SUDPTRL = LSB(&cdc_line_coding);
            SUDPTRCTL |= bmSDPAUTO;
            return true;
        case CDC_SET_CONTROL_LINE_STATE:
            cdc_line_state = SETUP_VALUE();
            return true;
    }
    return false;
}

void usb_cdc_tick(void) {
    while (!(EP2468STAT & bmEP4EMPTY)) {
        OUTPKTEND = 0x84;
        SYNCDELAY;
    }
}

void usb_cdc_puts(const char *str) {
    /* wait for free space in EP8 */
    while (EP2468STAT & bmEP8FULL);

    /* write str to the EP8 FIFO */
    strcpy(EP8FIFOBUF, str);

    /* send the data to the host */
    size_t len = strlen(str);
    EP8BCH = MSB(len);
    SYNCDELAY;
    EP8BCL = LSB(len);
    SYNCDELAY;
}
