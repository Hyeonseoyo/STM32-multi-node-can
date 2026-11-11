#ifndef SPEED_SENSOR_H
#define SPEED_SENSOR_H

#include <stdint.h>

void speed_sensor_init(void);
uint16_t speed_sensor_get_kmh(void);

#endif