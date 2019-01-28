#include <fx2macros.h>

#include "i2c.h"

void i2c_init(void) {
    I2CTL |= bm400KHZ;
}

void i2c_transfer(struct i2c_msg *msgs, size_t len) {
    struct i2c_msg *next_msg;
    size_t msg_len;

retry:
    next_msg = msgs;
    msg_len = len;

    I2CS |= bmSTART;

    while (msg_len--) {
        struct i2c_msg *msg = next_msg++;

        if (I2CS & bmBERR) {
            /* TODO: start timer */
        }

        I2DAT = msg->address;
        while (!(I2CS & bmDONE)); /* TODO: wait for timer */

        if (I2CS & bmBERR) {
            goto retry;
        } else if (!(I2CS & bmACK)) {
            break;
        }

        uint8_t *buf = msg->buf;
        size_t buf_len = msg->len;

        if (msg->address & I2C_READ) {
            if (buf_len == 1) {
                I2CS |= bmLASTRD;
            }
            (void) I2DAT;

            while (buf_len--) {
                while (!(I2CS & bmDONE));

                if (I2CS & bmBERR) {
                    goto retry;
                }

                if (buf_len == 1) {
                    I2CS |= bmLASTRD;
                } else if (buf_len == 0) {
                    if (msg_len) {
                        I2CS |= bmSTART;
                    } else {
                        I2CS |= bmSTOP;
                    }
                }
                *buf++ = I2DAT;
            }
        } else {
            while (buf_len--) {
                I2DAT = *buf++;
                while (!(I2CS & bmDONE));

                if (I2CS & bmBERR) {
                    goto retry;
                } else if (!(I2CS & bmACK)) {
                    break;
                }
            }

            if (msg_len) {
                I2CS |= bmSTART;
            } else {
                I2CS |= bmSTOP;
            }
        }
    }

    while (I2CS & bmSTOP);
}
