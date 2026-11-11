// APB1 clock = 16 MHz
// PA11 - CAN1_RX - AF9(CAN1)
// PA12 - CAN1_TX - AF9(CAN1)

// CAN1 base address 0x4000 6400

#include "can.h"

#define CAN_ID_VEHICLE_SPEED    0x200U
#define CAN_ID_TEMPERATURE      0x210U

static void can1_gpio_init(void)
{
    // RCC_AHB1ENR
    // 1: IO port A clock enabled
    *(volatile uint32_t*) 0x40023830U |= (0b1U << 0);       // GPIOAEN = 1

    // GPIOA_MODER
    // 10: Alternate function mode
    *(volatile uint32_t*) 0x40020000U &= ~(0b11U << 22);    // bit clear
    *(volatile uint32_t*) 0x40020000U &= ~(0b11U << 24);    // bit clear
    *(volatile uint32_t*) 0x40020000U |= (0b10U << 22);     // MODER11[1:0] = 10
    *(volatile uint32_t*) 0x40020000U |= (0b10U << 24);     // MODER12[1:0] = 10

    // GPIOA_OTYPER
    // 0: Output push-pull
    *(volatile uint32_t*) 0x40020004U &= ~(0b1U << 11);     // OT11 = 0
    *(volatile uint32_t*) 0x40020004U &= ~(0b1U << 12);     // OT12 = 0

    // GPIOA_OSPEEDR
    // 11: High speed
    *(volatile uint32_t*) 0x40020008U |= (0b11U << 22);     // OSPEEDR11[1:0] = 11
    *(volatile uint32_t*) 0x40020008U |= (0b11U << 24);     // OSPEEDR12[1:0] = 11

    // GPIOA_PUPDR
    // 00: No pull-up, pull-down
    *(volatile uint32_t*) 0x4002000CU &= ~(0b11U << 22);    // PUPDR11[1:0] = 00
    *(volatile uint32_t*) 0x4002000CU &= ~(0b11U << 24);    // PUPDR12[1:0] = 00

    // GPIOA_AFRH
    // 1001: AF9
    *(volatile uint32_t*) 0x40020024U &= ~(0b1111U << 12);  // bit clear
    *(volatile uint32_t*) 0x40020024U &= ~(0b1111U << 16);  // bit clear
    *(volatile uint32_t*) 0x40020024U |= (0b1001U << 12);   // AFRH11[3:0] = 1001
    *(volatile uint32_t*) 0x40020024U |= (0b1001U << 16);   // AFRH12[3:0] = 1001
}

static void can1_peripheral_init(void)
{
    // RCC_APB1ENR
    // 1: CAN 1 clock enabled
    *(volatile uint32_t*) 0x40023840U |= (0b1U << 25);      // CAN1EN = 1

    // CAN_MCR
    // 0: Exit Sleep mode
    *(volatile uint32_t*) 0x40006400U &= ~(0b1U << 1);      // SLEEP = 0
    // CAN_MSR
    while(*(volatile uint32_t*) 0x40006404U & (0b1U << 1))
    {
        // Wait until SLAK = 0
    }

    // CAN_MCR
    // 1: Initialization request
    *(volatile uint32_t*) 0x40006400U |= (0b1U << 0);       // INRQ = 1
    // CAN_MSR
    while(!(*(volatile uint32_t*) 0x40006404U & (0b1U <<0)))
    {
        // Wait until INAK = 1
    }

    // CAN_MCR
    // 0: Priority driven by the identifier of the message
    *(volatile uint32_t*) 0x40006400U &= ~(0b1U << 2);      // TXFP = 0
    // 0: Receive FIFO not locked on overrun. Once a receive FIFO is full the next incoming message overwrites the previous one
    *(volatile uint32_t*) 0x40006400U &= ~(0b1U << 3);      // RFLM = 0
    // 0: The CAN hardware automatically retransmits the message until it has been successfully transmitted according to the CAN standard
    *(volatile uint32_t*) 0x40006400U &= ~(0b1U << 4);      // NART = 0
    // 0: The Sleep mode is left on software request by clearing the SLEEP bit of the CAN_MCR register
    *(volatile uint32_t*) 0x40006400U &= ~(0b1U << 5);      // AWUM = 0
    // 1: The Bus-Off state is left automatically by hardware once 128 occurrences of 11 recessive bits have been monitored
    *(volatile uint32_t*) 0x40006400U |= (0b1U << 6);       // ABOM = 1
    // 0: Time Triggered Communication mode disabled
    *(volatile uint32_t*) 0x40006400U &= ~(0b1U << 7);      // TTCM = 0

    // CAN_BTR
    // // BRP = 1 -> Prescaler = 2
    *(volatile uint32_t*) 0x4000641CU &= ~(0b1111111111U << 0); // clear
    *(volatile uint32_t*) 0x4000641CU |= (0b0000000001U << 0);  // BRP[9:0] = 0000000001 = 1
    // TS1 = 12 -> BS1 = 13 tq
    *(volatile uint32_t*) 0x4000641CU &= ~(0b1111U << 16);      // clear
    *(volatile uint32_t*) 0x4000641CU |= (0b1100U << 16);       // TS1[3:0] = 1100 = 12
    // TS2 = 1 -> BS2 = 2 tq
    *(volatile uint32_t*) 0x4000641CU &= ~(0b111U << 20);       // clear
    *(volatile uint32_t*) 0x4000641CU |= (0b001U << 20);        // TS2[2:0] = 001 = 1
    // SJW = 0 -> SJW = 1 tq
    *(volatile uint32_t*) 0x4000641CU &= ~(0b11U << 24);        // SJW[1:0] = 00 = 0
    // 0: Loop back mode disabled
    *(volatile uint32_t*) 0x4000641CU &= ~(0b1U << 30);         // LBKM = 0
    // 0: Normal operation
    *(volatile uint32_t*) 0x4000641CU &= ~(0b1U << 31);         // SILM = 0

    // CAN_MCR
    // 0: switch the hardware into normal mode
    *(volatile uint32_t*) 0x40006400U &= ~(0b1U << 0);      // INRQ = 0
    // CAN_MSR
    while(*(volatile uint32_t*) 0x40006404U & (0b1U << 0))
    {
        // Wait until INAK = 0
    }
}

static void can1_filter_init(void)
{
    // CAN_FMR
    // 1: Initialization mode for the filters
    *(volatile uint32_t*) 0x40006600U |= (0b1U << 0);       // FINIT = 1

    // CAN_FA1R
    // 0: Filter x is not active
    *(volatile uint32_t*) 0x4000661CU &= ~(0b1U << 0);      // FACT0 = 0

    // CAN_FMR
    // 0: Active filters mode
    *(volatile uint32_t*) 0x40006600U &= ~(0b1U << 0);      // FINIT = 0
}

void can1_init(void)
{
    can1_gpio_init();
    can1_peripheral_init();
    can1_filter_init();
}

int can_send(uint16_t id, uint8_t *data, uint8_t dlc)
{
    uint32_t mailbox_offset;

    // Standard ID: 11-bit
    if(id > 0x7FFU)
    {
        return 0;
    }

    // CAN Data Length: 0~8 byte
    if(dlc > 8U)
    {
        return 0;
    }

    // DLC >= 1 인데 data가 NULL이면 전송 x
    if((dlc > 0U) && (data == 0))
    {
        return 0;
    }

    // CAN_TSR
    // TME0 = bit 26
    // TME1 = bit 27
    // TME2 = bit 28
    // 1: Transmit mailbox is empty
    uint32_t TSR = *(volatile uint32_t*) 0x40006408U;

    if(TSR & (0b1U << 26))
    {
        // mailbox 0
        mailbox_offset = 0x180U;
    }
    else if(TSR & (0b1U << 27))
    {
        // mailbox 1
        mailbox_offset = 0x190U;
    }
    else if(TSR & (0b1U << 28))
    {
        // mailbox 2
        mailbox_offset = 0x1A0U;
    }
    else
    {
        // 사용 가능한 TX mailbox 없음
        return 0;
    }

    volatile uint32_t *TIR = (volatile uint32_t*)(0x40006400U + mailbox_offset);
    volatile uint32_t *TDTR = (volatile uint32_t*)(0x40006400U + mailbox_offset + 0x04U);
    volatile uint32_t *TDLR = (volatile uint32_t*)(0x40006400U + mailbox_offset + 0x08U);
    volatile uint32_t *TDHR = (volatile uint32_t*)(0x40006400U + mailbox_offset + 0x0CU);

    // CAN_TIxR
    // STID[10:0] = bit 31:21
    // IDE = 0: Standard identifier
    // RTR = 0: Data frame
    // TXRQ = 0: 아직 전송요청 안함
    *TIR = ((uint32_t)id << 21);

    // CAN_TDTxR
    // DLC[3:0] = bit 3:0
    *TDTR = (uint32_t)(dlc & 0x0FU);

    // CAN_TDLxR
    // DATA0 ~ DATA3
    uint32_t low_data = 0U;

    if(dlc > 0U) low_data |= ((uint32_t)data[0] << 0);
    if(dlc > 1U) low_data |= ((uint32_t)data[1] << 8);
    if(dlc > 2U) low_data |= ((uint32_t)data[2] << 16);
    if(dlc > 3U) low_data |= ((uint32_t)data[3] << 24);

    *TDLR = low_data;

    // CAN_TDHxR
    // DATA4 ~ DATA7
    uint32_t high_data = 0U;

    if(dlc > 4U) high_data |= ((uint32_t)data[4] << 0);
    if(dlc > 5U) high_data |= ((uint32_t)data[5] << 8);
    if(dlc > 6U) high_data |= ((uint32_t)data[6] << 16);
    if(dlc > 7U) high_data |= ((uint32_t)data[7] << 24);

    *TDHR = high_data;

    // CAN_TIxR
    // TXRQ = 1: Request Transmission
    *TIR |= (0b1U << 0);

    return 1;
}

int node2_send_vehicle_speed(uint8_t speed)
{
    uint8_t data[1];

    data[0] = speed;

    return can_send(CAN_ID_VEHICLE_SPEED, data, 1);
}

int node2_send_temperature(float temperature)
{
    uint8_t data[2];

    int16_t temperature_data = (int16_t)(temperature * 100.0f);

    uint16_t raw_data = (uint16_t)temperature_data;

    data[0] = (uint8_t)(raw_data & 0xFFU);
    data[1] = (uint8_t)((raw_data >> 8) & 0xFFU);

    return can_send(CAN_ID_TEMPERATURE, data, 2);
}