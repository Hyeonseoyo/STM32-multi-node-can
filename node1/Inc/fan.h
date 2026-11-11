#ifndef FAN_H
#define FAN_H

#include <stdint.h>

void set_fan(void);
void fan_control(uint8_t command);

#endif