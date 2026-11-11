#include "fan.h"

// RCC base address     0x4002 3800U
// GPIOB base address   0x4002 0400U

void set_fan(void)
{
    // fan system use PB0
    
    // RCC_AHB1ENR
    // GPIOBEN = 1: IO port B clock enabled
    *(volatile uint32_t*) (0x40023830U) |= (0b1U << 1);

    // GPIOx_MODER
    // MODERy[1:0] = 01: General purpose output mode
    *(volatile uint32_t*) (0x40020400U) &= ~(0b11U << 0);
    *(volatile uint32_t*) (0x40020400U) |= (0b01U << 0);

    // GPIOx_OTYPER
    // OT0 = 0: Output push-pull
    *(volatile uint32_t*) (0x40020404U) &= ~(0b1U << 0);

    // GPIOx_OSPEEDR
    // OSPEEDR0[1:0] = 00: Low speed
    *(volatile uint32_t*) (0x40020408U) &= ~(0b11U << 0);

    // GPIOx_PUPDR
    // PUPDR0[1:0] = 00: No pull-up, pull-down
    *(volatile uint32_t*) (0x4002040CU) &= ~(0b11U << 0);

    // after init, fan off
    // GPIOx_BSRR
    // BR0 = 1: Resets the corresponding ODRx bit
    *(volatile uint32_t*) (0x40020418U) = (0b1U << 16);
}

static void fan_on(void)
{
    // BS0 = 1: Sets the corresponding ODRx bit
    *(volatile uint32_t*) (0x40020418U) = (0b1U << 0);
}

static void fan_off(void)
{
    // // BS0 = 0: No action on the corresponding ODRx bitx
    // *(volatile uint32_t*) (0x40020418U) = (0b0U << 0);
    // BR0 = 1: Resets the corresponding ODRx bit
    *(volatile uint32_t*) 0x40020418U = (0b1U << 16);
}

void fan_control(uint8_t command)
{
    if(command)
    {
        fan_on();
    }
    else
    {
        fan_off();
    }
}