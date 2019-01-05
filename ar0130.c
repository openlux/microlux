#include <delay.h>
#include <fx2macros.h>

#include "ar0130.h"
#include "i2c.h"

#define AR0130_ADDR 0x20

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
    /* set A0 (trigger) to output and set to low */
    OEA |= bmBIT0;
    IOA &= ~bmBIT0;

    /* bypass the PLL */
    ar0130_write(0x30B0, ar0130_read(0x30B0) | 0x4000);

    /* enable parallel interface and streaming mode */
    ar0130_write(0x301A, ar0130_read(0x301A) | 0x0084);
}

void ar0130_trigger(void) {
    /* raise trigger for at least 3 cycles */
    IOA |= bmBIT0;
    NOP;
    NOP;
    NOP;
    IOA &= ~bmBIT0;
}
