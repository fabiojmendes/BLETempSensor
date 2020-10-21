#pragma once

#include <stdint.h>

int volt_sensor_init();

int volt_sensor_read(int16_t *value);
