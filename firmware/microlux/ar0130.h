#ifndef AR0130_H
#define AR0130_H

#include <stdbool.h>
#include <stdint.h>

void ar0130_init(void);
bool ar0130_handle_command(uint8_t cmd);

#endif
