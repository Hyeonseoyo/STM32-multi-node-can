#ifndef UART_H
#define UART_H

void uart_send_char(char data);
void uart_send_string(const char *str);
void uart_send_int(int num);
void uart_send_temperature(float temperature);

void uart_init(void);

#endif