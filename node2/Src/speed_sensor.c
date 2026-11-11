#include <stdint.h>
#include "speed_sensor.h"

#define ADC_MAX_VALUE 255U
#define SPEED_MAX_KMH 200U

// RCC base address 0x4002 3800U
// GPIOA base address 0x4002 0000U
// Common ADC base address 0x4001 2300U
// ADC1 base address 0x4001 2000U

static void clock_enable_GPIOA(void)
{
    // RCC_AHB1ENR
    // GPIOAEN = 1: IO port A clock enabled
    *(volatile uint32_t*) (0x40023830U) |= (0b1U << 0);
}

static void clock_enable_adc(void)
{
    // RCC_APB2ENR
    // ADC1EN = 1: ADC1 clock enabled
    *(volatile uint32_t*) (0x40023844U) |= (0b1U << 8);
}

static void set_analog_mode_PA0(void)
{
    // GPIO_MODER
    // MODER0[1:0] = 11: Analog mode
    *(volatile uint32_t*) (0x40020000U) &= ~(0b11U << 0);
    *(volatile uint32_t*) (0x40020000U) |= (0b11U << 0);

    // GPIO_PUPDR
    // PUPDR0[1:0] = 00: No pull-up, pull-down
    *(volatile uint32_t*) (0x4002000CU) &= ~(0b11U << 0);
}

static void set_common_adc(void)
{
    // ADC_CCR
    // ADCPRE[1:0] = 00: PCLK2 divided by 2
    *(volatile uint32_t*) (0x40012304U) &= ~(0b11U << 16);
}

static void set_adc(void)
{
    // ADC_CR2
    *(volatile uint32_t*) (0x40012008U) &= ~(0b1U << 0);        // ADON = 0: Disable ADC conversion and go to power down mode

    // ADC_CR1
    *(volatile uint32_t*) (0x40012004U) &= ~(0b11U << 24);      // clear
    *(volatile uint32_t*) (0x40012004U) |= (0b10U << 24);       // RES[1:0] = 10: 8-bit(minimum 11 ADCCLK cycles)

    // ADC_CR2
    *(volatile uint32_t*) (0x40012008U) |= (0b1U << 1);         // CONT = 1: Continuous conversion mode
    *(volatile uint32_t*) (0x40012008U) &= ~(0b1U << 11);       // ALIGN = 0: Right alignment
    *(volatile uint32_t*) (0x40012008U) &= ~(0b1U << 10);       // EOCS = 0: The EOC bit is set at the end of the regular conversion sequence
    *(volatile uint32_t*) (0x40012008U) &= ~(0b11U << 28);       // EXTEN = 0: Trigger detection disabled
    
    // ADC_SMPR2
    // SMP0[2:0] = 100: 84 cycles
    *(volatile uint32_t*) (0x40012010U) &= ~(0b111U << 0);
    *(volatile uint32_t*) (0x40012010U) |= (0b100U << 0);

    // ADC_SQR1
    // L[3:0] = 0000: 1 conversion
    *(volatile uint32_t*) (0x4001202CU) &= ~(0b1111U << 20);

    // ADC_SQR3
    // SQ1[4:0] = 00000: 1st conversion is channel 0 (PA0, ADC1_IN0)
    *(volatile uint32_t*) (0x40012034U) &= ~(0b11111U << 0);
        
    *(volatile uint32_t*) (0x40012008U) |= (0b1U << 0);         // ADON = 1: Enable ADC
}

static void start_adc(void)
{
    for(volatile int i = 0U; i < 100U; i++)
    {
        // short stabilization delay
    }

    // ADC_CR2
    // SWSTART = 1: Starts conversion of regular channels
    *(volatile uint32_t*) (0x40012008U) |= (1U << 30);             
}

static uint16_t read_adc(void)
{
    while (((*(volatile uint32_t*)0x40012000U) & (0b1U << 1)) == 0U)
    {
        // wait until ADC1_SR.EOC = 1
    }

    return (uint16_t)(*(volatile uint32_t*)0x4001204CU);
}

uint16_t speed_sensor_get_kmh(void)
{
    uint16_t adc_value = read_adc();

    return (uint16_t)(((uint32_t)adc_value * SPEED_MAX_KMH) / ADC_MAX_VALUE);
}

void speed_sensor_init(void)
{
    clock_enable_GPIOA();
    clock_enable_adc();
    set_analog_mode_PA0();
    set_common_adc();
    set_adc();
    start_adc();
}