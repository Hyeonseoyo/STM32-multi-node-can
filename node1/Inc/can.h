#ifndef CAN_H
#define CAN_H

#include <stdint.h>

void can1_init(void);

int can_send(uint16_t id, uint8_t*data, uint8_t dlc);
int can_receive(uint16_t *id, uint8_t *data, uint8_t *dlc);

int node1_send_brake_status(uint8_t status);
int node1_send_fan_status(uint8_t status);
void node1_receive_command(uint32_t now_ms);

void can_poll_tx_result(void);
void can_poll_error_status(void);
void can_handle_error(void);

void node1_check_brake_timeout(uint32_t now_ms);

#endif