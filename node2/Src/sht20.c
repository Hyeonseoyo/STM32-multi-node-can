#include <stdint.h>
#include "sht20.h"
#include "delay.h"

static void sht20_trigger_temperature(void);

static void sht20_read_temperature_raw
(
    uint8_t *msb,
    uint8_t *lsb,
    uint8_t *crc
);

void clock_enable_GPIOB(void)
{
    // RCC base addresss 0x40023800U
    // RCC AHB1 peripheral clock enable register (RCC_AHB1ENR)
    *(volatile uint32_t*) (0x40023830U) |= (1U << 1);                       // bit set 1: IO port B clock enabled
}

void clock_enable_i2c(void)
{
    // RCC base addresss 0x40023800U
    // RCC APB1 peripheral clock enable register (RCC_APB1ENR)
    *(volatile uint32_t*) (0x40023840U) |= ((1U << 21));
}

void set_alternate_mode_PB8_PB9(void)
{
    // GPIOB base address 0x40020400U
    // GPIO port mode register (GPIOx_MODER)
    *(volatile uint32_t*) (0x40020400U) &= ~((0x3U << 16) | (0x3U << 18));  // bit clear
    *(volatile uint32_t*) (0x40020400U) |= ((0x2U << 16) | (0x2U << 18));   // bit set 10: Alternate function mode

    // GPIO port output type register (GPIOx_OTYPER)
    *(volatile uint32_t*) (0x40020404U) &= ~((0x1U << 8) | (0x1U << 9));    // bit clear
    *(volatile uint32_t*) (0x40020404U) |= ((0x1U << 8)| (0x1U << 9));      // bit set 1: Output open-drain

    // GPIO port output speed register (GPIOx_OSPEEDR)
    *(volatile uint32_t*) (0x40020408U) &= ~((0x3U << 16) | (0x3U << 18));  // bit clear
    *(volatile uint32_t*) (0x40020408U) |= ((0x3U << 16) | (0x3U << 18));   // bit set 11: High speed

    // GPIO port pull-up/pull-down register (GPIOx_PUPDR)
    *(volatile uint32_t*) (0x4002040CU) &= ~((0x3U << 16) | (0x3U << 18));  // bit clear
    *(volatile uint32_t*) (0x4002040CU) |= ((0x0U << 16) | (0x0U << 18));   // bit set 00: No pull-up, pull-down

    // GPIO alternate function high register (GPIOx_AFRH)
    *(volatile uint32_t*) (0x40020424U) &= ~((0xFU << 0) | (0xFU << 4));    // bit clear
    *(volatile uint32_t*) (0x40020424U) |= ((0x4U << 0) | (0x4U << 4));     // bit set 0100: AF4 (I2C1 -> AF4)
}

void set_i2c_PB8_PB9(void)
{
    // I2C1 base address 0x40005400U
    // I2C control register 1 (I2C_CR1)
    *(volatile uint32_t*) (0x40005400U) &= ~(0x1U << 0);    // bit clear and set 0: Peripheral disable
    *(volatile uint32_t*) (0x40005400U) |= (0x1U << 10);    // bit set 1: Acknowledge returned after a byte is received (matched address or data)

    // I2C control register 2 (I2C_CR2)
    // set FREQ
    *(volatile uint32_t*) (0x40005404U) &= ~(0x3FU << 0);   // bit clear
    *(volatile uint32_t*) (0x40005404U) |= (0x10U << 0);    // bit set 0b010000: 16MHz(= APB1 clock)
    
    // I2C clock control register (I2C_CCR) 4000 541CU
    // set CCR
    *(volatile uint32_t*) (0x4000541CU) &= ~(0xFFFU << 0);   // bit clear
    *(volatile uint32_t*) (0x4000541CU) |= (0x50U << 0);     // bit set 80: CCR = 80
    *(volatile uint32_t*) (0x4000541CU) &= ~(0x1U << 15);   // bit clear and set 0: Sm mode I2C

    // I2C TRISE register (I2C_TRISE) 4000 5420
    // set TRISE
    *(volatile uint32_t*) (0x40005420U) &= ~(0x3FU << 0);    // bit clear
    *(volatile uint32_t*) (0x40005420U) |= (0x11U << 0);     // bit set 17: TRISE = FREQ(=16) + 1

    // I2C1 enable
    *(volatile uint32_t*) (0x40005400U) |= (0x1U << 0);     // Peripheral enable 
}

void sht20_trigger_temperature(void)
{
    // I2C control register (I2C_CR1)
    *(volatile uint32_t*) (0x40005400U) |= (0x1U << 8);             // bit 8 [START] = 1: Repeated start generation

    // I2C status register 1 (I2C_SR1)
    while(!(*(volatile uint32_t*) (0x40005414U) & (0x1U << 0)))     // if bit 0 [SB] = 1, break loop
    {
        // wait until SB = 1
    }

    // I2C data register (I2C_DR)
    *(volatile uint32_t*) (0x40005410U) = (0x40U << 1);             // send SHT20 address + Write

    // I2C status register 1 (I2C_SR1)
    while(!(*(volatile uint32_t*) (0x40005414U) & (0x1U << 1)))     // if bit 1 [ADDR] = 1, break loop
    {
        // wait until ADDR = 1
    }

    // ADDR clear: SR1 read, SR2 read
    // I2C status register 1 (I2C_SR1)
    (void)*(volatile uint32_t*) (0x40005414U);                      // just read SR1 register

    // I2C status register 2 (I2C_SR2)
    (void)*(volatile uint32_t*) (0x40005418U);                      // just read SR2 register

    // I2C data register (I2C_DR)
    *(volatile uint32_t*) (0x40005410U) = 0xF3U;                    // send 11110011: Trigger T measure, no hold master

    // I2C status register (I2C_SR1)
    while(!(*(volatile uint32_t*) (0x40005414U) & (0x1U << 7)))
    {
        // wait until TxE = 1
    }
    
    while(!(*(volatile uint32_t*) (0x40005414U) & (0x1U << 2)))
    {
        // wait until BTF = 1
    }
    
    *(volatile uint32_t*) (0x40005400U) |= (0x1U << 9);
}

void sht20_read_temperature_raw(uint8_t *msb, uint8_t *lsb, uint8_t *crc)
{
    // ACK enable
    *(volatile uint32_t*) (0x40005400U) |= (0x1U << 10);            // I2C_CR1 bit 10 [ACK] = 1, Acknowledge returned after a byte is received

    // POS disable
    *(volatile uint32_t*) (0x40005400U) &= ~(0x1U << 11);

    // Start 생성
    *(volatile uint32_t*) (0x40005400U) |= (0x1U << 8);             // I2C_CR1 bit 8 [START] = 1, Repeated start generation

    // wait untill SB = 1
    while(!(*(volatile uint32_t*) (0x40005414U) & (0x1U << 0)))
    {
        // wait until SB = 1
    }

    // SHT20 address + read command
    *(volatile uint32_t*) (0x40005410U) = ((0x40U << 1) | (0x1U));

    // wait ADDR = 1
    while(!(*(volatile uint32_t*) (0x40005414U) & (0x1U << 1)))
    {

    }

    // ADDR clear: SR1 read, SR2 read
    (void)*(volatile uint32_t*) (0x40005414U);
    (void)*(volatile uint32_t*) (0x40005418U);

    // wait until two bytes are received
    while(!(*(volatile uint32_t*) (0x40005414U) & (0x1U << 2)))
    {
        // wait BTF = 1
    }

    // Now 3 bytes remain.
    // Disable ACK before reading N-2 byte.
    *(volatile uint32_t*) (0x40005400U) &= ~(0x1U << 10);

    // Read first byte: MSB
    *msb = *(volatile uint32_t*) (0x40005410U);

    // wait until last two bytes are ready
    while(!(*(volatile uint32_t*) (0x40005414U) & (0x1U << 2)))
    {
        // wait BTF = 1
    }
    
    // Generate STOP before reading last two bytes
    *(volatile uint32_t*) (0x40005400U) |= (0x1U << 9);

    // Read second byte: LSB
    *lsb = *(volatile uint32_t*) (0x40005410U);

    // Read third byte: CRC
    *crc = *(volatile uint32_t*) (0x40005410U);

    // Restore ACK for next communication
    *(volatile uint32_t*) (0x40005400U) |= (0x1U << 10);
}

float sht20_measure_temperature(void)
{
    uint8_t msb = 0;
    uint8_t lsb = 0;
    uint8_t crc = 0;

    uint16_t raw_data = 0;
    float temperature = 0.0;

    // 온도 측정 시작 명령 보내기
    sht20_trigger_temperature();

    // 측정 시간 기다리기
    delay_ms(100);

    // 측정된 raw 데이터 읽기
    sht20_read_temperature_raw(&msb, &lsb, &crc);

    // MSB, LSB 합쳐서 16bit raw data 만들기
    raw_data = (((uint16_t)msb << 8) | lsb);

    // 하위 2비트 status bit 제거
    raw_data &= ~0x0003U;

    // raw data로 온도 계산
    temperature = -46.85f + (175.72f * ((float)raw_data / 65536.0f));

    return temperature;
}

void sht20_init(void)
{
    clock_enable_GPIOB();
    clock_enable_i2c();
    set_alternate_mode_PB8_PB9();
    set_i2c_PB8_PB9();
}