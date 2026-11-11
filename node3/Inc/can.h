#ifndef CAN_H
#define CAN_H

#include <stdint.h>

void can_init(void);

int can_send(uint16_t id, uint8_t*data, uint8_t dlc);
int can_receive(uint16_t *id, uint8_t *data, uint8_t *dlc);

void node3_send_brake_cmd(uint8_t command);
void node3_send_fan_cmd(uint8_t command);
void node3_receive_sensor_data(void);

#endif