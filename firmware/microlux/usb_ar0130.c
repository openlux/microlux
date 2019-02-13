#include <fx2macros.h>
#include <string.h>

#include "ar0130.h"
#include "usb_ar0130.h"

#define AR0130_CTRL_EXPOSURE 0x80

bool usb_ar0130_handle_command(uint8_t cmd) {
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
