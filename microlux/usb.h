#ifndef USB_H
#define USB_H

#include <stdbool.h>

extern volatile bool usb_configured;

void usb_init(void);
void usb_tick(void);

#endif
