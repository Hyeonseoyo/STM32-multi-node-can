#ifndef OLED_H
#define OLED_H

#include <stdint.h>

void oled_init(void);
void oled_clear(void);

void oled_write_string_center(uint8_t area_x,
                              uint8_t area_width,
                              uint8_t page,
                              const char *string,
                              uint8_t scale);

// Fixed field with blank background. Returns 1 on successful I2C writes.
// scale: 1 or 2. page: 0..7, with room for the full field height.
int oled_write_field_center(uint8_t area_x, uint8_t area_width,
                            uint8_t page, const char *text, uint8_t scale);

#endif