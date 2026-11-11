#include <stdint.h>
#include "i2c.h"
#include "oled.h"

#define OLED_ADDRESS 0x3C

static int oled_write_commands(const uint8_t *commands, uint16_t length);

static void oled_write_char(uint8_t x,
                            uint8_t page,
                            char character,
                            uint8_t scale);

static void oled_write_string(uint8_t x,
                              uint8_t page,
                              const char *string,
                              uint8_t scale);

// 5 x 7 uppercase font
static const uint8_t font_upper[26][5] =
{
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}  // Z
};

// 5 x 7 number font
static const uint8_t font_number[10][5] =
{
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}  // 9
};

// Decimal point
static const uint8_t font_dot[5] =
{
    0x00, 0x60, 0x60, 0x00, 0x00
};

// Degree symbol
// '@' is used internally to represent °
static const uint8_t font_degree[5] =
{
    0x06, 0x09, 0x09, 0x06, 0x00
};

// Space overwrites old pixels; '-' supports negative temperatures and '--'.
static const uint8_t font_space[5] = {0, 0, 0, 0, 0};
static const uint8_t font_minus[5] = {0x08, 0x08, 0x08, 0x08, 0x08};

static const uint8_t *oled_get_font(char character)
{
    if(character >= 'A' && character <= 'Z') return font_upper[character - 'A'];
    if(character >= '0' && character <= '9') return font_number[character - '0'];
    if(character == '.') return font_dot;
    if(character == '@') return font_degree;
    if(character == '-') return font_minus;
    if(character == ' ') return font_space;
    return 0;
}

static int oled_write_commands(const uint8_t *commands, uint16_t length)
{
    i2c1_start();

    if (!i2c1_write_address(OLED_ADDRESS))
    {
        i2c1_stop();
        return 0;
    }

    // Control byte:
    // following bytes are commands
    if (!i2c1_write_byte(0x00))
    {
        i2c1_stop();
        return 0;
    }

    for (uint16_t i = 0; i < length; i++)
    {
        if (!i2c1_write_byte(commands[i]))
        {
            i2c1_stop();
            return 0;
        }
    }

    i2c1_stop();

    return 1;
}

void oled_init(void)
{
    const uint8_t commands[] =
    {
        0xAE,       // Display OFF

        0x00,       // Lower column
        0x10,       // Higher column
        0x40,       // Display start line
        0xB0,       // Page 0

        0x81, 0xCF, // Contrast
        0xA1,       // Segment remap
        0xA6,       // Normal display
        0xA8, 0x3F, // Multiplex ratio
        0xC8,       // COM scan direction
        0xD3, 0x00, // Display offset
        0xD5, 0x80, // Clock divide
        0xD9, 0xF1, // Pre-charge
        0xDA, 0x12, // COM pins
        0xDB, 0x30, // VCOMH
        0x8D, 0x14, // Charge pump enable

        0xAF        // Display ON
    };

    if(!oled_write_commands(commands, sizeof(commands))) return;
}

void oled_clear(void)
{
    const uint8_t commands[] =
    {
        0x20, 0x00,        // Horizontal Addressing Mode
        0x21, 0x00, 0x7F,  // Column 0 ~ 127
        0x22, 0x00, 0x07   // Page 0 ~ 7
    };

    if(!oled_write_commands(commands, sizeof(commands))) return;

    // Now write 1024 zero bytes continuously
    i2c1_start();

    if (!i2c1_write_address(OLED_ADDRESS))
    {
        i2c1_stop();
        return;
    }

    // following bytes are display data
    if (!i2c1_write_byte(0x40))
    {
        i2c1_stop();
        return;
    }

    for (uint16_t i = 0; i < 1024; i++)
    {
        if (!i2c1_write_byte(0x00))
        {
            i2c1_stop();
            return;
        }
    }

    i2c1_stop();
}

static void oled_write_char(uint8_t x, uint8_t page, char character, uint8_t scale)
{
    const uint8_t *font;

    // Scale = 1 ~ 3
    if (scale < 1 || scale > 3)
    {
        return;
    }

    font = oled_get_font(character);
    if(font == 0) return;

    // Character size
    uint8_t width = 6 * scale;      // 5 columns + 1 spacing
    uint8_t height = 7 * scale;

    // Number of OLED pages required
    uint8_t pages = (height + 7) / 8;

    // Display boundary check
    if ((x + width) > 128)
    {
        return;
    }

    if ((page + pages) > 8)
    {
        return;
    }

    // Set drawing area
    const uint8_t commands[] =
    {
        0x20, 0x00,                         // Horizontal Addressing Mode
        0x21, x, (uint8_t)(x + width - 1), // Column range
        0x22, page, (uint8_t)(page + pages - 1)
    };

    if(!oled_write_commands(commands, sizeof(commands))) return;

    // Start data transmission
    i2c1_start();

    if (!i2c1_write_address(OLED_ADDRESS))
    {
        i2c1_stop();
        return;
    }

    // Following bytes are display data
    if (!i2c1_write_byte(0x40))
    {
        i2c1_stop();
        return;
    }

    // OLED sends data page by page
    for (uint8_t p = 0; p < pages; p++)
    {
        // Original character = 5 columns
        // + 1 blank spacing column
        for (uint8_t column = 0; column < 6; column++)
        {
            uint32_t scaled_column = 0;

            if (column < 5)
            {
                uint8_t original_column = font[column];

                // Vertical scaling
                for (uint8_t bit = 0; bit < 7; bit++)
                {
                    if (original_column & (1U << bit))
                    {
                        for (uint8_t v = 0; v < scale; v++)
                        {
                            scaled_column |=
                                (1UL << (bit * scale + v));
                        }
                    }
                }
            }

            // Extract one OLED page (8 pixels)
            uint8_t data =
                (scaled_column >> (p * 8)) & 0xFF;

            // Horizontal scaling
            for (uint8_t h = 0; h < scale; h++)
            {
                if (!i2c1_write_byte(data))
                {
                    i2c1_stop();
                    return;
                }
            }
        }
    }

    i2c1_stop();
}

static void oled_write_string(uint8_t x, uint8_t page, const char *string, uint8_t scale)
{
    uint16_t current_x = x;

    // Scale = 1 ~ 3
    if (scale < 1 || scale > 3)
    {
        return;
    }

    while (*string != '\0')
    {
        // Character width:
        // 5 pixel font + 1 pixel spacing
        uint8_t width = 6 * scale;

        // Stop if the next character exceeds the display width
        if ((current_x + width) > 128)
        {
            return;
        }

        oled_write_char((uint8_t)current_x, page, *string, scale);
        current_x += width;

        // Move to next character
        string++;
    }
}

void oled_write_string_center(uint8_t area_x,
                              uint8_t area_width,
                              uint8_t page,
                              const char *string,
                              uint8_t scale)
{
    if (scale < 1 || scale > 3)
    {
        return;
    }

    uint16_t length = 0;

    if(string == 0) return;
    while (string[length] != '\0')
    {
        if(length >= 21U) return;
        length++;
    }

    uint16_t text_width = length * 6 * scale;

    if (text_width > area_width)
    {
        return;
    }

    uint8_t x = area_x + (area_width - text_width) / 2;

    oled_write_string(x, page, string, scale);
}
/* Write a complete fixed field, including its blank background, in one transfer.
 * scale 1 = one page, scale 2 = two pages. Max local buffer: 256 bytes.
 * No full-screen clear is required when text shrinks or changes alignment.
 */
int oled_write_field_center(uint8_t area_x, uint8_t area_width,
                            uint8_t page, const char *text, uint8_t scale)
{
    uint8_t pixels[256] = {0};
    uint16_t length = 0U;
    uint8_t pages;
    uint16_t text_width;
    uint16_t start_x;

    if(text == 0 || area_width == 0U || scale < 1U || scale > 2U) return 0;
    pages = (uint8_t)((7U * scale + 7U) / 8U);
    if((uint16_t)area_x + area_width > 128U || (uint16_t)page + pages > 8U) return 0;

    while(text[length] != '\0')
    {
        if(length >= area_width / (6U * scale)) return 0;
        if(oled_get_font(text[length]) == 0) return 0;
        length++;
    }
    text_width = length * 6U * scale;
    start_x = (area_width - text_width) / 2U;

    for(uint16_t i = 0U; i < length; i++)
    {
        const uint8_t *font = oled_get_font(text[i]);
        for(uint8_t column = 0U; column < 5U; column++)
        {
            for(uint8_t bit = 0U; bit < 7U; bit++)
            {
                if((font[column] & (1U << bit)) == 0U) continue;
                for(uint8_t v = 0U; v < scale; v++)
                {
                    uint8_t y = (uint8_t)(bit * scale + v);
                    for(uint8_t h = 0U; h < scale; h++)
                    {
                        uint16_t x = start_x + i * 6U * scale + column * scale + h;
                        pixels[(y / 8U) * area_width + x] |= (uint8_t)(1U << (y % 8U));
                    }
                }
            }
        }
    }

    const uint8_t commands[] = {
        0x20, 0x00,
        0x21, area_x, (uint8_t)(area_x + area_width - 1U),
        0x22, page, (uint8_t)(page + pages - 1U)
    };
    if(!oled_write_commands(commands, sizeof(commands))) return 0;
    i2c1_start();
    if(!i2c1_write_address(OLED_ADDRESS) || !i2c1_write_byte(0x40U))
    {
        i2c1_stop();
        return 0;
    }
    for(uint16_t i = 0U; i < (uint16_t)area_width * pages; i++)
    {
        if(!i2c1_write_byte(pixels[i]))
        {
            i2c1_stop();
            return 0;
        }
    }
    i2c1_stop();
    return 1;
}












// #include <stdint.h>
// #include "i2c.h"
// #include "oled.h"

// #define OLED_ADDRESS 0x3C

// static int oled_write_commands(const uint8_t *commands, uint16_t length);

// static void oled_write_char(uint8_t x,
//                             uint8_t page,
//                             char character,
//                             uint8_t scale);

// static void oled_write_string(uint8_t x,
//                               uint8_t page,
//                               const char *string,
//                               uint8_t scale);

// // 5 x 7 uppercase font
// static const uint8_t font_upper[26][5] =
// {
//     {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
//     {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
//     {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
//     {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
//     {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
//     {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
//     {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
//     {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
//     {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
//     {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
//     {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
//     {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
//     {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
//     {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
//     {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
//     {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
//     {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
//     {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
//     {0x46, 0x49, 0x49, 0x49, 0x31}, // S
//     {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
//     {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
//     {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
//     {0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
//     {0x63, 0x14, 0x08, 0x14, 0x63}, // X
//     {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
//     {0x61, 0x51, 0x49, 0x45, 0x43}  // Z
// };

// // 5 x 7 number font
// static const uint8_t font_number[10][5] =
// {
//     {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
//     {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
//     {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
//     {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
//     {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
//     {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
//     {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
//     {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
//     {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
//     {0x06, 0x49, 0x49, 0x29, 0x1E}  // 9
// };

// // Decimal point
// static const uint8_t font_dot[5] =
// {
//     0x00, 0x60, 0x60, 0x00, 0x00
// };

// // Degree symbol
// // '@' is used internally to represent °
// static const uint8_t font_degree[5] =
// {
//     0x06, 0x09, 0x09, 0x06, 0x00
// };

// static int oled_write_commands(const uint8_t *commands, uint16_t length)
// {
//     i2c1_start();

//     if (!i2c1_write_address(OLED_ADDRESS))
//     {
//         i2c1_stop();
//         return 0;
//     }

//     // Control byte:
//     // following bytes are commands
//     if (!i2c1_write_byte(0x00))
//     {
//         i2c1_stop();
//         return 0;
//     }

//     for (uint16_t i = 0; i < length; i++)
//     {
//         if (!i2c1_write_byte(commands[i]))
//         {
//             i2c1_stop();
//             return 0;
//         }
//     }

//     i2c1_stop();

//     return 1;
// }

// void oled_init(void)
// {
//     const uint8_t commands[] =
//     {
//         0xAE,       // Display OFF

//         0x00,       // Lower column
//         0x10,       // Higher column
//         0x40,       // Display start line
//         0xB0,       // Page 0

//         0x81, 0xCF, // Contrast
//         0xA1,       // Segment remap
//         0xA6,       // Normal display
//         0xA8, 0x3F, // Multiplex ratio
//         0xC8,       // COM scan direction
//         0xD3, 0x00, // Display offset
//         0xD5, 0x80, // Clock divide
//         0xD9, 0xF1, // Pre-charge
//         0xDA, 0x12, // COM pins
//         0xDB, 0x30, // VCOMH
//         0x8D, 0x14, // Charge pump enable

//         0xAF        // Display ON
//     };

//     oled_write_commands(commands, sizeof(commands));
// }

// void oled_clear(void)
// {
//     const uint8_t commands[] =
//     {
//         0x20, 0x00,        // Horizontal Addressing Mode
//         0x21, 0x00, 0x7F,  // Column 0 ~ 127
//         0x22, 0x00, 0x07   // Page 0 ~ 7
//     };

//     oled_write_commands(commands, sizeof(commands));

//     // Now write 1024 zero bytes continuously
//     i2c1_start();

//     if (!i2c1_write_address(OLED_ADDRESS))
//     {
//         i2c1_stop();
//         return;
//     }

//     // following bytes are display data
//     if (!i2c1_write_byte(0x40))
//     {
//         i2c1_stop();
//         return;
//     }

//     for (uint16_t i = 0; i < 1024; i++)
//     {
//         if (!i2c1_write_byte(0x00))
//         {
//             i2c1_stop();
//             return;
//         }
//     }

//     i2c1_stop();
// }

// static void oled_write_char(uint8_t x, uint8_t page, char character, uint8_t scale)
// {
//     const uint8_t *font;

//     // Scale = 1 ~ 3
//     if (scale < 1 || scale > 3)
//     {
//         return;
//     }

//     // A ~ Z
//     if (character >= 'A' && character <= 'Z')
//     {
//         font = font_upper[character - 'A'];
//     }

//     // 0 ~ 9
//     else if (character >= '0' && character <= '9')
//     {
//         font = font_number[character - '0'];
//     }

//     // Decimal point
//     else if (character == '.')
//     {
//         font = font_dot;
//     }

//     // Degree symbol
//     else if (character == '@')
//     {
//         font = font_degree;
//     }

//     // Unsupported character
//     else
//     {
//         return;
//     }

//     // Character size
//     uint8_t width = 6 * scale;      // 5 columns + 1 spacing
//     uint8_t height = 7 * scale;

//     // Number of OLED pages required
//     uint8_t pages = (height + 7) / 8;

//     // Display boundary check
//     if ((x + width) > 128)
//     {
//         return;
//     }

//     if ((page + pages) > 8)
//     {
//         return;
//     }

//     // Set drawing area
//     const uint8_t commands[] =
//     {
//         0x20, 0x00,                         // Horizontal Addressing Mode
//         0x21, x, (uint8_t)(x + width - 1), // Column range
//         0x22, page, (uint8_t)(page + pages - 1)
//     };

//     oled_write_commands(commands, sizeof(commands));

//     // Start data transmission
//     i2c1_start();

//     if (!i2c1_write_address(OLED_ADDRESS))
//     {
//         i2c1_stop();
//         return;
//     }

//     // Following bytes are display data
//     if (!i2c1_write_byte(0x40))
//     {
//         i2c1_stop();
//         return;
//     }

//     // OLED sends data page by page
//     for (uint8_t p = 0; p < pages; p++)
//     {
//         // Original character = 5 columns
//         // + 1 blank spacing column
//         for (uint8_t column = 0; column < 6; column++)
//         {
//             uint32_t scaled_column = 0;

//             if (column < 5)
//             {
//                 uint8_t original_column = font[column];

//                 // Vertical scaling
//                 for (uint8_t bit = 0; bit < 7; bit++)
//                 {
//                     if (original_column & (1U << bit))
//                     {
//                         for (uint8_t v = 0; v < scale; v++)
//                         {
//                             scaled_column |=
//                                 (1UL << (bit * scale + v));
//                         }
//                     }
//                 }
//             }

//             // Extract one OLED page (8 pixels)
//             uint8_t data =
//                 (scaled_column >> (p * 8)) & 0xFF;

//             // Horizontal scaling
//             for (uint8_t h = 0; h < scale; h++)
//             {
//                 if (!i2c1_write_byte(data))
//                 {
//                     i2c1_stop();
//                     return;
//                 }
//             }
//         }
//     }

//     i2c1_stop();
// }

// static void oled_write_string(uint8_t x, uint8_t page, const char *string, uint8_t scale)
// {
//     uint16_t current_x = x;

//     // Scale = 1 ~ 3
//     if (scale < 1 || scale > 3)
//     {
//         return;
//     }

//     while (*string != '\0')
//     {
//         // Character width:
//         // 5 pixel font + 1 pixel spacing
//         uint8_t width = 6 * scale;

//         // Stop if the next character exceeds the display width
//         if ((current_x + width) > 128)
//         {
//             return;
//         }

//         // Space
//         if (*string == ' ')
//         {
//             current_x += width;
//         }
//         else
//         {
//             oled_write_char(
//                 (uint8_t)current_x,
//                 page,
//                 *string,
//                 scale
//             );

//             current_x += width;
//         }

//         // Move to next character
//         string++;
//     }
// }

// void oled_write_string_center(uint8_t area_x,
//                               uint8_t area_width,
//                               uint8_t page,
//                               const char *string,
//                               uint8_t scale)
// {
//     if (scale < 1 || scale > 3)
//     {
//         return;
//     }

//     uint8_t length = 0;

//     while (string[length] != '\0')
//     {
//         length++;
//     }

//     uint16_t text_width = length * 6 * scale;

//     if (text_width > area_width)
//     {
//         return;
//     }

//     uint8_t x = area_x + (area_width - text_width) / 2;

//     oled_write_string(x, page, string, scale);
// }