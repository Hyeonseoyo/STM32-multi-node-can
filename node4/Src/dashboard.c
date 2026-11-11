// Node4 dashboard: CAN data interpretation and OLED screen layout.
#include <stdint.h>
#include "dashboard.h"
#include "can.h"
#include "oled.h"

#define CAN_ID_VEHICLE_SPEED  0x200U
#define CAN_ID_TEMPERATURE    0x210U
#define CAN_ID_BRAKE_STATUS   0x300U
#define CAN_ID_FAN_STATUS     0x310U

// Field order: speed, temperature, brake, fan.
static uint8_t speed;
static int16_t temperature_x100;
static uint8_t brake_status;
static uint8_t fan_status;
static uint8_t valid_mask;
static uint8_t dirty_mask = 0x0FU;
static uint8_t next_field;

// Caller provides at least 6 bytes. Works for all uint16_t values.
static void format_uint(uint16_t value, char *text)
{
    char reverse[5];
    uint8_t count = 0U;
    do
    {
        reverse[count++] = (char)('0' + value % 10U);
        value /= 10U;
    } while(value != 0U);
    while(count != 0U) *text++ = reverse[--count];
    *text = '\0';
}

// CAN unit is 0.01 C. Display one decimal, truncated toward zero.
// int32_t intermediate also handles INT16_MIN and values between -1 and 0.
static void format_temperature(int16_t value, char *text)
{
    int32_t magnitude = value;
    if(magnitude < 0)
    {
        *text++ = '-';
        magnitude = -magnitude;
    }
    format_uint((uint16_t)(magnitude / 100), text);
    while(*text != '\0') text++;
    *text++ = '.';
    *text++ = (char)('0' + (magnitude / 10) % 10);
    *text = '\0';
}

static void dashboard_receive_pending(void)
{
    uint16_t id;
    uint8_t data[8];
    uint8_t dlc;

    // Drain the FIFO before each display field. This is still polling:
    // frames may be lost during a blocking OLED transfer; RX IRQ is next.
    while(can_receive(&id, data, &dlc))
    {
        uint8_t bit;
        uint8_t changed;
        if(id == CAN_ID_VEHICLE_SPEED && dlc == 1U && data[0] <= 200U)
        {
            bit = 0x01U;
            changed = (speed != data[0]);
            speed = data[0];
        }
        else if(id == CAN_ID_TEMPERATURE && dlc == 2U)
        {
            uint16_t raw = (uint16_t)(((uint16_t)data[1] << 8) | data[0]);
            int32_t decoded = raw;
            if(raw & 0x8000U) decoded -= 65536;
            bit = 0x02U;
            changed = (temperature_x100 != decoded);
            temperature_x100 = (int16_t)decoded;
        }
        else if(id == CAN_ID_BRAKE_STATUS && dlc == 1U && data[0] <= 1U)
        {
            bit = 0x04U;
            changed = (brake_status != data[0]);
            brake_status = data[0];
        }
        else if(id == CAN_ID_FAN_STATUS && dlc == 1U && data[0] <= 1U)
        {
            bit = 0x08U;
            changed = (fan_status != data[0]);
            fan_status = data[0];
        }
        else continue;

        if(changed || (valid_mask & bit) == 0U) dirty_mask |= bit;
        valid_mask |= bit;
    }
}

static void dashboard_update_one_field(void)
{
    char text[8];
    const char *display_text = "--";

    // Fair round-robin: one field per loop, then return to CAN reception.
    for(uint8_t n = 0U; n < 4U; n++)
    {
        uint8_t field = next_field;
        uint8_t bit = (uint8_t)(1U << field);
        next_field = (uint8_t)((next_field + 1U) % 4U);
        if((dirty_mask & bit) == 0U) continue;

        if(valid_mask & bit)
        {
            if(field == 0U)
            {
                format_uint(speed, text);
                display_text = text;
            }
            else if(field == 1U)
            {
                format_temperature(temperature_x100, text);
                display_text = text;
            }
            else if(field == 2U) display_text = brake_status ? "ON" : "OFF";
            else display_text = fan_status ? "ON" : "OFF";
        }

        uint8_t x = (field == 0U || field == 2U) ? 0U : 56U;
        uint8_t width = (x == 0U) ? 56U : 72U;
        uint8_t page = (field < 2U) ? 2U : 7U;
        uint8_t scale = (field < 2U) ? 2U : 1U;
        if(oled_write_field_center(x, width, page, display_text, scale))
        {
            dirty_mask &= (uint8_t)~bit;
        }
        return;
    }
}

// Call after set_i2c() and oled_init().
void dashboard_init(void)
{
    speed = 0U;
    temperature_x100 = 0;
    brake_status = 0U;
    fan_status = 0U;
    valid_mask = 0U;
    dirty_mask = 0x0FU;
    next_field = 0U;

    oled_clear();

    // Static labels once. Pages are eight-pixel-high rows, numbered 0..7.
    oled_write_string_center(0U, 56U, 0U, "SPEED", 1U);
    oled_write_string_center(56U, 72U, 0U, "TEMP", 1U);
    oled_write_string_center(0U, 56U, 4U, "KMH", 1U);
    oled_write_string_center(56U, 72U, 4U, "@C", 1U);
    oled_write_string_center(0U, 56U, 6U, "BRAKE", 1U);
    oled_write_string_center(56U, 72U, 6U, "FAN", 1U);
    for(uint8_t i = 0U; i < 4U; i++) dashboard_update_one_field();

}

// Call repeatedly after can_init(). No deliberate delay is added.
void dashboard_process(void)
{
    dashboard_receive_pending();
    dashboard_update_one_field();
}
