#include <delay.h>
#include <fx2ints.h>
#include <fx2macros.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "ar0130.h"
#include "i2c.h"
#include "usb.h"
#include "usb_fifo.h"

#define AR0130_ADDR 0x20

#define AR0130_CTRL_EXPOSURE 0x80

struct ar0130_exposure_config {
    uint16_t x_start;
    uint16_t x_end;
    uint16_t y_start;
    uint16_t y_end;

    uint8_t gain;
    uint8_t offset;

    uint16_t duration_coarse;
    uint16_t duration_fine;
    uint16_t line_width;
};

struct ar0130_exposure_config exposure_config;

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

static void ar0130_start_exposure(struct ar0130_exposure_config *new_config) {
    bool reset = memcmp(&exposure_config, new_config, sizeof(exposure_config));

    ar0130_write(0x3004, new_config->x_start);
    ar0130_write(0x3008, new_config->x_end);
    ar0130_write(0x3002, new_config->y_start);
    ar0130_write(0x3006, new_config->y_end);

    ar0130_write(0x305E, new_config->gain);
    ar0130_write(0x301E, new_config->offset);

    ar0130_write(0x3012, new_config->duration_coarse);
    ar0130_write(0x3014, new_config->duration_fine);
    ar0130_write(0x300C, new_config->line_width);

    memcpy(&exposure_config, new_config, sizeof(exposure_config));

    if (reset) {
        ar0130_write(0x301A, ar0130_read(0x301A) | 0x0002);
    }
}

void ar0130_init(void) {
    /* bypass the PLL */
    ar0130_write(0x30B0, ar0130_read(0x30B0) | 0x4000);

    /* enable parallel interface and standby_eof */
    ar0130_write(0x301A, ar0130_read(0x301A) | 0x0294);
}

bool ar0130_handle_command(uint8_t cmd) {
    if (cmd == AR0130_CTRL_EXPOSURE) {
        struct ar0130_exposure_config new_config;
        size_t len = sizeof(new_config);

        EP0BCL = 0;
        while (EP0BCL < len);

        memcpy(&new_config, EP0BUF, len);

        ar0130_start_exposure(&new_config);

        return true;
    }

    return false;
}
