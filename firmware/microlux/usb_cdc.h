#ifndef USB_CDC_H
#define USB_CDC_H

#include <stdbool.h>
#include <stdint.h>

bool usb_cdc_handle_command(uint8_t cmd);
void usb_cdc_tick(void);
void usb_cdc_puts(const char *str);

#endif
