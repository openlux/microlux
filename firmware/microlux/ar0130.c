#include <stdbool.h>
#include <string.h>
#include <fx2macros.h>
#include <delay.h>

#include "ar0130.h"
#include "microlux.h"
#include "i2c.h"
#include "usb_fifo.h"

#define AR0130_ADDR 0x20

#define AR0130_REGISTER_RESET 0x301A
#define AR0130_RESET_DEFAULT  0x00C8

#define AR0130_STREAM 0x0004
#define AR0130_RESET  0x0001

#define AR0130_REGISTER_PLL 0x30B0
#define AR0130_PLL_DEFAULT  0x5000

#define AR0130_REGISTER_X_START 0x3004
#define AR0130_REGISTER_X_END   0x3008
#define AR0130_REGISTER_Y_START 0x3002
#define AR0130_REGISTER_Y_END   0x3006

#define AR0130_REGISTER_GAIN   0x305E
#define AR0130_REGISTER_OFFSET 0x301E

#define AR0130_REGISTER_EXPOSURE_DURATION 0x3012
#define AR0130_REGISTER_LINE_WIDTH        0x300C

struct ar0130_exposure_config exposure_config;

bool exposing = false;

static uint16_t ar0130_read(uint16_t reg) {
    uint8_t buf[] = {
        (uint8_t) (reg >> 8),
        (uint8_t) reg
    };
    struct i2c_msg msgs[] = {
        {
            .address = AR0130_ADDR,
            .buf = &buf,
            .len = sizeof(buf)
        },
        {
            .address = AR0130_ADDR | I2C_READ,
            .buf = &buf,
            .len = sizeof(buf)
        }
    };
    i2c_transfer(msgs, sizeof(msgs) / sizeof(msgs[0]));
    return (buf[0] << 8) | buf[1];
}

static void ar0130_write(uint16_t reg, uint16_t value) {
    uint8_t buf[] = {
        (uint8_t) (reg >> 8),
        (uint8_t) reg,
        (uint8_t) (value >> 8),
        (uint8_t) value
    };
    struct i2c_msg msgs[] = {
        {
            .address = AR0130_ADDR,
            .buf = &buf,
            .len = sizeof(buf)
        }
    };
    i2c_transfer(msgs, sizeof(msgs) / sizeof(msgs[0]));
}

void ar0130_init(void) {
    /* bypass the PLL */
    ar0130_write(AR0130_REGISTER_PLL, AR0130_PLL_DEFAULT);

    /* enable parallel interface and disable stby_eof */
    ar0130_write(AR0130_REGISTER_RESET, AR0130_RESET_DEFAULT);
}

void ar0130_start_exposure(struct ar0130_exposure_config *new_config) {
    /* using memcmp() is fine here as structs are packed by default in sdcc */
    bool reset = memcmp(&exposure_config, new_config, sizeof(exposure_config));

    /* if exposure duration is >250ms (2097 lines) then reset the sensor to abort current exposure */
    if (exposing && reset && (new_config->duration_coarse > 2097)) {
        ar0130_stop_exposure();
    }

    ar0130_write(AR0130_REGISTER_X_START, new_config->x_start);
    ar0130_write(AR0130_REGISTER_X_END,   new_config->x_end);
    ar0130_write(AR0130_REGISTER_Y_START, new_config->y_start);
    ar0130_write(AR0130_REGISTER_Y_END,   new_config->y_end);

    ar0130_write(AR0130_REGISTER_GAIN,    new_config->gain);
    ar0130_write(AR0130_REGISTER_OFFSET,  new_config->offset);

    ar0130_write(AR0130_REGISTER_EXPOSURE_DURATION, new_config->duration_coarse);
    ar0130_write(AR0130_REGISTER_LINE_WIDTH,        new_config->line_width);

    memcpy(&exposure_config, new_config, sizeof(exposure_config));

    if (!exposing) {
        /* bypass the PLL */
        ar0130_write(AR0130_REGISTER_PLL, AR0130_PLL_DEFAULT);

        /* enable parallel interface, disable stby_eof and enable streaming */
        ar0130_write(AR0130_REGISTER_RESET, AR0130_RESET_DEFAULT | AR0130_STREAM);

        /* switch to external IFCLK */
        IFCONFIG = 0;
	SYNCDELAY;

        /* ensure EP2 is configured */
        EP2CFG = bmVALID | bmDIR | bmTYPE1;
        SYNCDELAY;

        usb_fifo_init();
    }

    exposing = true;
}

void ar0130_stop_exposure(void) {
    /* switch to internal 48 MHz IFCLK */
    IFCONFIG = bmIFCLKSRC | bm3048MHZ;
    SYNCDELAY;

    /* TODO disabiling EP2 here seems to break something */
    //EP2CFG &= ~bmVALID;
    //SYNCDELAY;

    /* wait for IFCLK switch */
    delay(1);

    /* reset AR0130, this aborts current exposure and disables streaming */
    ar0130_write(AR0130_REGISTER_RESET, AR0130_RESET_DEFAULT | AR0130_RESET);

    /* wait for sensor to start up, TODO longer duration? */
    delay(1);

    exposing = false;
}
