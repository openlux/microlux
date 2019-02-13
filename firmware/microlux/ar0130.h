#ifndef AR0130_H
#define AR0130_H

#include <stdbool.h>
#include <stdint.h>

struct exposure_config {
    uint16_t x_start;
    uint16_t x_end;
    uint16_t y_start;
    uint16_t y_end;

    uint8_t gain;
    uint8_t offset;

    uint16_t duration_coarse;
    uint16_t duration_fine;
    uint16_t line_width;
};

bool camera_handle_command(uint8_t cmd);
void start_exposure(struct exposure_config *new_config);
bool compare_config(struct exposure_config *new_config);
void ar0130_init(void);

#endif
