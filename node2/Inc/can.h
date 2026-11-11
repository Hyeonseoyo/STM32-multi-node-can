#ifndef CAN_H
#define CAN_H

#include <stdint.h>

void can1_init(void);

int can_send(uint16_t id, uint8_t*data, uint8_t dlc);

int node2_send_vehicle_speed(uint8_t speed);
int node2_send_temperature(float temperature);

#endif