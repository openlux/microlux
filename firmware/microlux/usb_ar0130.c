#include <fx2macros.h>
#include <string.h>

#include "ar0130.h"
#include "usb_ar0130.h"

#define AR0130_START_EXPOSURE 0x80
#define AR0130_STOP_EXPOSURE  0x81

bool usb_ar0130_handle_command(uint8_t cmd) {
    struct ar0130_exposure_config new_config;
    size_t len = sizeof(new_config);
    switch (cmd) {
        case AR0130_START_EXPOSURE:
            EP0BCL = 0;
            while (EP0BCL < len);

            memcpy(&new_config, EP0BUF, len);

            ar0130_start_exposure(&new_config);
            return true;
	case AR0130_STOP_EXPOSURE:
	    ar0130_stop_exposure();
	    return true;
    }
    return false;
}
