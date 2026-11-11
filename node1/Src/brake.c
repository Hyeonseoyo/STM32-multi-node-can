#include "brake.h"

// RCC base address     0x4002 3800U
// GPIOB base address   0x4002 0400U

void set_brake(void)
{
    // brake system use PB4

    // RCC_AHB1ENR
    // GPIOBEN = 1: IO port B clock enabled
    *(volatile uint32_t*) (0x40023830U) |= (0b1U << 1);

    // GPIOx_MODER
    // MODERy[1:0] = 01: General purpose output mode
    *(volatile uint32_t*) (0x40020400U) &= ~(0b11U << 8);
    *(volatile uint32_t*) (0x40020400U) |= (0b01U << 8);

    // GPIOx_OTYPER
    // OT4 = 0: Output push-pull
    *(volatile uint32_t*) (0x40020404U) &= ~(0b1U << 4);

    // GPIOx_OSPEEDR
    // OSPEEDR4[1:0] = 00: Low speed
    *(volatile uint32_t*) (0x40020408U) &= ~(0b11U << 8);

    // GPIOx_PUPDR
    // PUPDR4[1:0] = 00: No pull-up, pull-down
    *(volatile uint32_t*) (0x4002040CU) &= ~(0b11U << 8);

    // after init, brake(=led) off
    // GPIOx_BSRR
    // BR4 = 1: Resets the corresponding ODRx bit
    *(volatile uint32_t*) (0x40020418U) = (0b1U << 20);
}

static void brake_on(void)
{
    // brake on = led on
    // GPIOx_BSRR
    // BS4 = 1: Sets the corresponding ODRx bit
    *(volatile uint32_t*) (0x40020418U) = (0b1U << 4);
}

static void brake_off(void)
{
    // brake off = led off
    // GPIOx_BSRR
    // BR4 = 1: Resets the corresponding ODRx bit
    *(volatile uint32_t*) (0x40020418U) = (0b1U << 20);
}

void brake_control(uint8_t command)
{
    if(command)
    {
        brake_on();
    }
    else
    {
        brake_off();
    }
}