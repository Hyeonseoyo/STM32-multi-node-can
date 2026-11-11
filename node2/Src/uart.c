#include <stdint.h>
#include "uart.h"

static void clock_enable_GPIOA(void)
{
    // RCC AHB1 peripheral clock enable register (RCC_AHB1ENR)
    *(volatile uint32_t*) (0x40023830) |= (0x1U << 0);          // set bit 0 [GPIOAEN] = 1, IO port A clock enabled
}

static void clock_enable_uart(void)
{
    // RCC APB1 peripheral clock enable register (RCC_APB1ENR)
    *(volatile uint32_t*) (0x40023840U) |= (0x1U << 17);        // set bit 17 [USART2EN] = 1, USART2 clock enabled
}

static void set_alternate_mode_PA2(void)
{
    // GPIOA base address 0x40020000U
    // GPIO port mode register (GPIOx_MODER)
    *(volatile uint32_t*) (0x40020000U) &= ~(0x3U << 4);        // bit clear
    *(volatile uint32_t*) (0x40020000U) |= (0x2U << 4);         // bit set 10: Alternate function mode

    // GPIO port output type register (GPIOx_OTYPER)
    *(volatile uint32_t*) (0x40020004U) &= ~(0x1U << 2);        // bit clear and set 0: Output push-pull

    // GPIO port output speed register (GPIOx_OSPEEDR)
    *(volatile uint32_t*) (0x40020008U) &= ~(0x3U << 4);        // bit clear
    *(volatile uint32_t*) (0x40020008U) |= (0x3U << 4);         // bit set 11: High speed

    // GPIO port pull-up/pull-down register (GPIOx_PUPDR)
    *(volatile uint32_t*) (0x4002000CU) &= ~(0x3U << 4);        // bit clear
    *(volatile uint32_t*) (0x4002000CU) |= (0x0U << 4);         // bit set 00: No pull-up, pull-down

    // GPIO alternate function high register (GPIOx_AFRL)
    *(volatile uint32_t*) (0x40020020U) &= ~(0xFU << 8);        // bit clear
    *(volatile uint32_t*) (0x40020020U) |= (0x7U << 8);         // bit set 0111: AF7 (USART2 -> AF7)
}

static void set_uart_PA2(void)
{
    // uart base address 0x40004400U
    // Control register 1 (USART_CR1)
    *(volatile uint32_t*) (0x4000440CU) &= ~(0x1U << 13);       // bit set 0: USART prescaler and outputs disabled

    // Baud rate register (USART_BRR)
    // USART2_BRR: baud rate = 115200
    // PCLK1 = 16MHz 기준
    *(volatile uint32_t*) (0x40004408U) = 0x8BU;                // bit set 139, USARTDIV = 16,000,000 / 115,200 = 약 138.888, 가장 가까운 정수는 139

    // Control register 1 (USART_CR1)
    *(volatile uint32_t*) (0x4000440CU) &= ~(0x1U << 12);       // bit set 0: 1 Start bit, 8 Data bits, n Stop bit

    // Control register 2 (USART_CR2)
    *(volatile uint32_t*) (0x40004410U) &= ~(0x3U << 12);       // bit set 00: 1 Stop bit

    // Control register 1 (USART_CR1)
    *(volatile uint32_t*) (0x4000440CU) &= ~(0x1U << 10);       // bit set 0: Parity control disabled

    // Control register 1 (USART_CR1)
    *(volatile uint32_t*) (0x4000440CU) |= (0x1U << 3);         // bit set 1: Transmitter is enabled

    // Control register 1 (USART_CR1)
    *(volatile uint32_t*) (0x4000440CU) |= (0x1U << 13);        // bit set 1: USART enabled
}

void uart_send_char(char data)
{
    // USART2_SR: TXE = 1이면 DR이 비어있다는 뜻
    while(!(*(volatile uint32_t*) (0x40004400U) & (0x1U << 7)))
    {
        // wait until TXE = 1
    }

    // USART2_DR: 보낼 데이터 쓰기
    *(volatile uint32_t*) (0x40004404U) = data;
}

void uart_send_string(const char *str)
{
    while(*str)
    {
        uart_send_char(*str);
        str++;
    }
}

void uart_send_int(int num)
{
    char buf[10];
    int i = 0;

    if(num == 0)
    {
        uart_send_char('0');
        return;
    }

    while(num > 0)
    {
        buf[i] = (num % 10) + '0';
        num = num / 10;
        i++;
    }

    while(i > 0)
    {
        i--;
        uart_send_char(buf[i]);
    }
}


void uart_send_temperature(float temperature)
{
    int integer_part = 0;
    int decimal_part = 0;

    integer_part = (int)temperature;
    decimal_part = (int)((temperature - integer_part) * 10.0f);

    uart_send_string("Temperature : ");
    uart_send_int(integer_part);
    uart_send_char('.');
    uart_send_char(decimal_part + '0');
    uart_send_string(" C\r\n");
}

void uart_init(void)
{
    clock_enable_GPIOA();
    clock_enable_uart();
    set_alternate_mode_PA2();
    set_uart_PA2();
}