#ifndef I2C_H
#define I2C_H

#include <stddef.h>
#include <stdint.h>

#define I2C_READ 0x1

struct i2c_msg {
    uint8_t address;
    void *buf;
    size_t len;
};

void i2c_init(void);
void i2c_transfer(struct i2c_msg *msgs, size_t len);

#endif
