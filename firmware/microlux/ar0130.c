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

#define SET_REGISTER 0x80
#define CTRL_EXPOSURE 0x80

struct exposure_config exposure_config;

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

bool camera_handle_command(uint8_t cmd) {
    if (cmd == CTRL_EXPOSURE) {
        struct exposure_config new_config;
        size_t len = sizeof(new_config);

        EP0BCL = 0;
        while (EP0BCL < len);

        memcpy(&new_config, EP0BUF, len);

        start_exposure(&new_config);

        return true;
    }

    return false;
}

void start_exposure(struct exposure_config *new_config) {
    bool reset = compare_config(new_config);

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

bool compare_config(struct exposure_config *new_config) {
    bool reset = false;

    if (new_config->x_start != exposure_config.x_start) reset = true;
    if (new_config->x_end   != exposure_config.x_end  ) reset = true;
    if (new_config->y_start != exposure_config.y_start) reset = true;
    if (new_config->y_end   != exposure_config.y_end  ) reset = true;

    if (new_config->gain   != exposure_config.gain  ) reset = true;
    if (new_config->offset != exposure_config.offset) reset = true;

    if (new_config->duration_coarse != exposure_config.duration_coarse) reset = true;
    if (new_config->duration_fine   != exposure_config.duration_fine  ) reset = true;
    if (new_config->line_width      != exposure_config.line_width     ) reset = true;

    return reset;
}

void ar0130_init(void) {
    /* bypass the PLL */
    ar0130_write(0x30B0, ar0130_read(0x30B0) | 0x4000);

    /* enable parallel interface and standby_eof */
    ar0130_write(0x301A, ar0130_read(0x301A) | 0x0294);

    /* enable parallel interface and streaming mode */
    //ar0130_write(0x301A, ar0130_read(0x301A) | 0x0084);

    // 0x3002 = y start, 0x3006 = y end (inc), 0x3004 = x start, 0x3008 = x end (inc), 0x3032 = binning

    /* line width */
    //ar0130_write(0x300C, 65535);

    /* exposure time */
    //ar0130_write(0x3012, 100); // coarse (lines)
    //ar0130_write(0x3014, 0); // fine (pixels)

    /* gain, max=255?, 0x301E = offset */
    //ar0130_write(0x305E, 127);

    /* restart */
    //ar0130_write(0x301A, ar0130_read(0x301A) | 0x0002);
}

