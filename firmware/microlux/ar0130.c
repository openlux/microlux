#include <stdbool.h>
#include <string.h>

#include "ar0130.h"
#include "i2c.h"

#define AR0130_ADDR 0x20

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

void ar0130_init(void) {
    /* bypass the PLL */
    ar0130_write(0x30B0, ar0130_read(0x30B0) | 0x4000);

    /* enable mask_bad, parallel interface and stdby_eof */
    ar0130_write(0x301A, ar0130_read(0x301A) | 0x0290);
}

void ar0130_start_exposure(struct ar0130_exposure_config *new_config) {
    /* using memcmp() is fine here as structs are packed by default in sdcc */
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

    uint16_t flags = 0x0004;

    if (reset) {
        flags |= 0x0002;
    }

    ar0130_write(0x301A, ar0130_read(0x301A) | flags);
}

void ar0130_stop_exposure(void) {
    ar0130_write(0x301A, (ar0130_read(0x301A) & ~0x0004) | 0x0002);
}
