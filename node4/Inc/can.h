#ifndef CAN_H
#define CAN_H

#include <stdint.h>

void can_init(void);

int can_receive(uint16_t *id, uint8_t *data, uint8_t *dlc);

void node4_receive_data(uint8_t *speed, int16_t *temperature_data, uint8_t *brake_status, uint8_t *fan_status);

#endif