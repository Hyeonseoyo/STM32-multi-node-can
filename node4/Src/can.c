// APB1 clock = 8 MHz
// PA11 - CANRX
// PA12 - CANTX

// CAN base address 0x4000 6400

#include "can.h"

#define CAN_ID_VEHICLE_SPEED    0x200U
#define CAN_ID_TEMPERATURE      0x210U
#define CAN_ID_BRAKE_STATUS     0x300U
#define CAN_ID_FAN_STATUS       0x310U

static void can_gpio_init(void)
{
    // RCC_APB2ENR
    // 1: Alternate Function IO clock enabled
    *(volatile uint32_t*) 0x40021018U |= (0b1U << 0);       // AFIOEN = 1

    // AFIO_MAPR
    // 00: CAN_RX mapped to PA11, CAN_TX mapped to PA12
    *(volatile uint32_t*) 0x40010004U &= ~(0b11U << 13);    // CAN_REMAP[1:0] = 00

    // RCC_APB2ENR
    // 1: IO port A clock enabled
    *(volatile uint32_t*) 0x40021018U |= (0b1U << 2);       // IOPAEN

    // For CAN_RX
    // GPIOA_CRH
    // 00: Input mode
    *(volatile uint32_t*) 0x40010804U &= ~(0b11U << 12);    // MODE11[1:0] = 00
    // 01: Floating input
    *(volatile uint32_t*) 0x40010804U &= ~(0b11U << 14);    // clear
    *(volatile uint32_t*) 0x40010804U |= (0b01U << 14);     // CNF11[1:0] = 01

    // For CAN_TX
    // GPIOA_CRH
    // 01: Output mode, max speed 10 MHz
    *(volatile uint32_t*) 0x40010804U &= ~(0b11U << 16);    // clear
    *(volatile uint32_t*) 0x40010804U |= (0b01U << 16);     // MODE12[1:0] = 01
    // 10: Alternate function output Push-pull
    *(volatile uint32_t*) 0x40010804U &= ~(0b11U << 18);    // clear
    *(volatile uint32_t*) 0x40010804U |= (0b10U << 18);     // CNF12[1:0] = 10
}

static void can_peripheral_init(void)
{
    // RCC_APB1ENR
    // 1: CAN clock enabled
    *(volatile uint32_t*) 0x4002101CU |= (0b1U << 25);      // CANEN = 1

    // CAN_MCR
    // 0: Exit Sleep mode
    *(volatile uint32_t*) 0x40006400U &= ~(0b1U << 1);      // SLEEP = 0
    // CAN_MSR
    while(*(volatile uint32_t*) 0x40006404U & (0b1U << 1))
    {
        // Wait until SLAK = 0
    }

    // CAN_MCR
    // 1: request to CAN hardware to enter initialization mode
    *(volatile uint32_t*) 0x40006400U |= (0b1U << 0);       // INRQ = 1
    // CAN_MSR
    while(!(*(volatile uint32_t*) 0x40006404U & (0b1U << 0)))
    {
        // Wait until INAK = 1
    }

    // CAN_MCR
    // 0: Priority driven by the identifier of the message
    *(volatile uint32_t*) 0x40006400U &= ~(0b1U << 2);      // TXFP = 0
    // 0: Receive FIFO not locked on overrun. Once a receive FIFO is full the next incoming message will overwrite the previous one
    *(volatile uint32_t*) 0x40006400U &= ~(0b1U << 3);      // RFLM = 0
    // 0: The CAN hardware will automatically retransmit the message until it has been successfully transmitted according to the CAN standard
    *(volatile uint32_t*) 0x40006400U &= ~(0b1U << 4);      // NART = 0
    // 0: The Sleep mode is left on software request by clearing the SLEEP bit of the CAN_MCR register
    *(volatile uint32_t*) 0x40006400U &= ~(0b1U << 5);      // AWUM = 0
    // 1: The Bus-Off state is left automatically by hardware once 128 occurrences of 11 recessive bits have been monitored
    *(volatile uint32_t*) 0x40006400U |= (0b1U << 6);       // ABOM = 1
    // 0: Time Triggered Communication mode disabled
    *(volatile uint32_t*) 0x40006400U &= ~(0b1U << 7);      // TTCM = 0

    // CAN_BTR
    // BRP = 0 -> Prescaler = 1
    *(volatile uint32_t*) 0x4000641CU &= ~(0b1111111111U << 0);     // BRP[9:0] = 0
    // TS1 = 12 -> BS1 = 13 tq
    *(volatile uint32_t*) 0x4000641CU &= ~(0b1111U << 16);          // clear
    *(volatile uint32_t*) 0x4000641CU |= (0b1100U << 16);           // TS1[3:0] = 1100 = 12
    // TS2 = 1 -> BS2 = 2 tq
    *(volatile uint32_t*) 0x4000641CU &= ~(0b111U << 20);           // clear
    *(volatile uint32_t*) 0x4000641CU |= (0b001U << 20);            // TS2 = 001 = 1
    // SJW = 0 -> SJW = 1 tq
    *(volatile uint32_t*) 0x4000641CU &= ~(0b11U << 24);            // SJW[1:0] = 0
    // 0: Loop Back Mode disabled
    *(volatile uint32_t*) 0x4000641CU &= ~(0b1U << 30);             // LBKM = 0
    // 0: Normal operation
    *(volatile uint32_t*) 0x4000641CU &= ~(0b1U << 31);             // SILM = 0

    // CAN_MCR
    // switch the hardware into normal mode
    *(volatile uint32_t*) 0x40006400U &= ~(0b1U << 0);              // INRQ = 0
    // CAN_MSR
    while(*(volatile uint32_t*) 0x40006404U & (0b1U << 0))
    {
        // Wait until INAK = 0
    }
}

static void can_filter_init(void)
{
    // CAN_FMR
    // 1: Initialization mode for the filters
    *(volatile uint32_t*) 0x40006600U |= (0b1U << 0);       // FINIT = 1

    // CAN_FA1R
    // 0: Filter x is not active
    *(volatile uint32_t*) 0x4000661CU &= ~(0b1U << 0);      // FACT0 = 0
    *(volatile uint32_t*) 0x4000661CU &= ~(0b1U << 1);      // FACT1 = 0

    // CAN_FM1R
    // 0: Two 32-bit registers of filter bank x are in Identifier List mode
    *(volatile uint32_t*) 0x40006604U |= (0b1U << 0);      // FBM0 = 1
    *(volatile uint32_t*) 0x40006604U |= (0b1U << 1);      // FBM1 = 1

    // CAN_FS1R
    // 1: Single 32-bit scale configuration
    *(volatile uint32_t*) 0x4000660CU |= (0b1U << 0);       // FSC0 = 1
    *(volatile uint32_t*) 0x4000660CU |= (0b1U << 1);       // FSC1 = 1

    // CAN_FFA1R
    // 0: Filter assigned to FIFO 0
    *(volatile uint32_t*) 0x40006614U &= ~(0b1U << 0);      // FFA0 = 0
    *(volatile uint32_t*) 0x40006614U &= ~(0b1U << 1);      // FFA1 = 0

    // Filter bank 0
    // STID[10:0] = bit 31:21
    // IDE = 0: Standard identifier
    // RTR = 0: Data frame
    // CAN_F0R1
    // First ID: CAN_ID_VEHICLE_SPEED
    *(volatile uint32_t*) 0x40006640U = ((uint32_t)CAN_ID_VEHICLE_SPEED << 21);
    // CAN_F0R2
    // Second ID: CAN_ID_TEMPERATURE
    *(volatile uint32_t*) 0x40006644U = ((uint32_t)CAN_ID_TEMPERATURE << 21);

    // Filter bank 1
    // STID[10:0] = bit 31:21
    // IDE = 0: Standard identifier
    // RTR = 0: Data frame
    // CAN_F1R1
    // First ID: CAN_ID_BRAKE_STATUS
    *(volatile uint32_t*) 0x40006648U = ((uint32_t)CAN_ID_BRAKE_STATUS << 21);
    // CAN_F1R2
    // Second ID: CAN-ID_FAN_STATUS
    *(volatile uint32_t*) 0x4000664CU = ((uint32_t)CAN_ID_FAN_STATUS << 21);

    // CAN_FA1R
    // 1: Filter x is active
    *(volatile uint32_t*) 0x4000661CU |= (0b1U << 0);       // FACT0 = 1
    *(volatile uint32_t*) 0x4000661CU |= (0b1U << 1);       // FACT1 = 1

    // CAN_FMR
    // 0: Active filters mode
    *(volatile uint32_t*) 0x40006600U &= ~(0b1U << 0);      // FINIT = 0
}


void can_init(void)
{
    can_gpio_init();
    can_peripheral_init();
    can_filter_init();
}

int can_receive(uint16_t *id, uint8_t *data, uint8_t *dlc)
{
    // CAN_RF0R
    // FMP0[1:0]
    // 00: No message pending in FIFO 0
    if((*(volatile uint32_t*) 0x4000640CU & 0b11U) == 0U)
    {
        return 0; 
    }

    // Receive FIFO mailbox 0 registers
    uint32_t RIR = *(volatile uint32_t*) 0x400065B0U;
    uint32_t RDTR = *(volatile uint32_t*) 0x400065B4U;
    uint32_t RDLR = *(volatile uint32_t*) 0x400065B8U;
    uint32_t RDHR = *(volatile uint32_t*) 0x400065BCU;

    // CAN_RI0R
    // STID[10:0] = bit 31:21
    *id = (uint16_t)((RIR >> 21) & 0x7FFU);

    // CAN_RDT0R
    // DLC[3:0] = bit 3:0
    *dlc = (uint8_t)((RDTR & 0xFU));

    // CAN_RDL0R
    // DATA0 ~ DATA3
    if(*dlc > 0U) data[0] = (uint8_t)((RDLR >> 0) & 0xFFU);
    if(*dlc > 1U) data[1] = (uint8_t)((RDLR >> 8) & 0xFFU);
    if(*dlc > 2U) data[2] = (uint8_t)((RDLR >> 16) & 0xFFU);
    if(*dlc > 3U) data[3] = (uint8_t)((RDLR >> 24) & 0xFFU);

    // CAN_RDH0R
    // DATA4 ~ DATA7
    if(*dlc > 4U) data[4] = (uint8_t)((RDHR >> 0) & 0xFFU);
    if(*dlc > 5U) data[5] = (uint8_t)((RDHR >> 8) & 0xFFU);
    if(*dlc > 6U) data[6] = (uint8_t)((RDHR >> 16) & 0xFFU);
    if(*dlc > 7U) data[7] = (uint8_t)((RDHR >> 24) & 0xFFU);

    // CAN_RF0R
    // RFOM0 = 1
    // Release FIFO 0 output mailbox
    *(volatile uint32_t*) 0x4000640CU |= (0b1U << 5);

    return 1;
}

void node4_receive_data(uint8_t *speed, int16_t *temperature_data, uint8_t *brake_status, uint8_t *fan_status)
{
    uint16_t id;
    uint8_t dlc;
    uint8_t data[8];

    if(!can_receive(&id, data, &dlc))
    {
        return;
    }

    if((id == CAN_ID_VEHICLE_SPEED) && (dlc == 1U))
    {
        *speed = data[0];
    }
    else if((id == CAN_ID_TEMPERATURE) && (dlc == 2U))
    {
        uint16_t raw_data = ((uint16_t)data[1] << 8) | (uint16_t)data[0];

        *temperature_data = (int16_t)raw_data;
    }
    else if((id == CAN_ID_BRAKE_STATUS) && (dlc == 1U))
    {
        *brake_status = data[0];
    }
    else if((id == CAN_ID_FAN_STATUS) && (dlc == 1U))
    {
        *fan_status = data[0];
    }
}
