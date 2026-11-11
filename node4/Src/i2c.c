#include "i2c.h"

// RCC base address     0x4002 1000
// GPIOB base address   0x4001 0C00
// I2C1 base address    0x4000 5400

// CLK reset value
    // 1. HSION = 1: internal 8 MHz RC oscillator ON
    // 2. SW[1:0] = 00: HSI selected as system clock
    // 3. presaler
    //  3-1. HPRE[3:0] = 0000: SYSCLK not divided
    //  3-2. PPRE1[2:0] = 000: HCLK not divided
    //  3-3. PPRE2[2:0] = 000: HCLK not divided
    // 4. CLK
    //  4-1. SYSCLK = 8 MHz
    //  4-2. HCLK   = 8 MHz
    //  4-3. PCLK1  = 8 MHz
    //  4-4. PCLK2  = 8 MHz

// T_SCL = T_high + T_low
//      = 2 × CCR × T_PCLK1
//
// f_SCL = 1 / T_SCL
//
// f_SCL = PCLK1 / (2 * CCR)

void set_i2c(void)
{
    // clock enable
    *(volatile uint32_t*) 0x4002101CU |= (0b1U << 21);      // i2c clock
    *(volatile uint32_t*) 0x40021018U |= (0b1U << 3);       // GPIOB clock

    // set PB6 : I2C1_SCL
    // CNF6[1:0] = 11: Alternate function output Open-drain
    *(volatile uint32_t*) 0x40010C00U |= (0b11 << 26);
    // MODE6[1:0] = 11: Output mode, max speed 50 MHz
    *(volatile uint32_t*) 0x40010C00U |= (0b11 << 24);

    // set PB7 : I2C1_SDA
    // CNF7[1:0] = 11: Alternate function output Open-drain
    *(volatile uint32_t*) 0x40010C00U |= (0b11 << 30);
    // MODE7[1:0] = 11: Output mode, max speed 50 MHz
    *(volatile uint32_t*) 0x40010C00U |= (0b11 << 28);

    // set I2C1
    // CR2 FREQ[5:0] = 8 MHz
    *(volatile uint32_t*) 0x40005404U &= ~(0b111111U << 0);
    *(volatile uint32_t*) 0x40005404U |= (8U << 0);
    // F/S = 0: Sm mode I2C
    *(volatile uint32_t*) 0x4000541CU &= ~(0b1U << 15);
    // CCR = 40
    *(volatile uint32_t*) 0x4000541CU &= ~(0xFFFU << 0);
    *(volatile uint32_t*) 0x4000541CU |= 40U;
    //TRISE[5:0] = 9
    *(volatile uint32_t*) 0x40005420U &= ~(0b111111U << 0);
    *(volatile uint32_t*) 0x40005420U |= 9U;

    // OAR1 bit 14 must be kept at 1
    *(volatile uint32_t*) 0x40005408U |= (0b1U << 14);

    // enable I2C1
    // PE = 1: Peripheral enable
    *(volatile uint32_t*) 0x40005400U |= (0b1U << 0);
}

void i2c1_start(void)
{
    // wait until I2C bus is idle
    // SR2.BUSY = 0
    while (*(volatile uint32_t*)0x40005418U & (1U << 1))
    {
    }

    // START = 1
    *(volatile uint32_t*) 0x40005400U |= (0b1U << 8);

    // wait until SB = 1
    while ((*(volatile uint32_t*) 0x40005414U & (0b1U << 0)) == 0)
    {
    }
}

int i2c1_write_address(uint8_t address)
{
    // Send slave address + Write bit
    *(volatile uint32_t*) 0x40005410U = (address << 1);

    while (1)
    {
        uint32_t sr1 = *(volatile uint32_t*) 0x40005414U;

        // ADDR = 1: address matched, ACK received
        if (sr1 & (0b1U << 1))
        {
            // Clear ADDR flag
            // Read SR1 followed by SR2
            volatile uint32_t temp;

            temp = *(volatile uint32_t*) 0x40005414U;
            temp = *(volatile uint32_t*) 0x40005418U;
            (void)temp;

            return 1;
        }

        // AF = 1: acknowledge failure
        if (sr1 & (0b1U << 10))
        {
            // Clear AF
            *(volatile uint32_t*) 0x40005414U &= ~(0b1U << 10);

            return 0;
        }
    }
}

int i2c1_write_byte(uint8_t data)
{
    // Write data to DR
    *(volatile uint32_t*) 0x40005410U = data;

    while (1)
    {
        uint32_t sr1 = *(volatile uint32_t*) 0x40005414U;

        // BTF = 1: byte transfer finished
        if (sr1 & (0b1U << 2))
        {
            return 1;
        }

        // AF = 1: NACK received
        if (sr1 & (0b1U << 10))
        {
            // Clear AF
            *(volatile uint32_t*) 0x40005414U &= ~(0b1U << 10);

            return 0;
        }
    }
}

void i2c1_stop(void)
{
    // STOP = 1: Stop generation
    *(volatile uint32_t*)0x40005400U |= (1U << 9);
}