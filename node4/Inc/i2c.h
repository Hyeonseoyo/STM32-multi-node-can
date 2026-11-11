#ifndef I2C_H
#define I2C_H

#include <stdint.h>

void set_i2c(void);
void i2c1_start(void);
int i2c1_write_address(uint8_t address);
int i2c1_write_byte(uint8_t data);
void i2c1_stop(void);

#endif