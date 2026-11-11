#include "delay.h"

void delay_ms(uint32_t ms)
{
    // SysTick base address: 0xE000E010
    // LOAD  = 0xE000E014
    // VAL   = 0xE000E018
    // CTRL  = 0xE000E010

    for(uint32_t i = 0; i < ms; i++)
    {
        // 16MHz 기준 1ms = 16000 clock
        *(volatile uint32_t*) (0xE000E014U) = 16000U - 1U;

        // current value clear
        *(volatile uint32_t*) (0xE000E018U) = 0U;

        // SysTick enable, processor clock 사용
        *(volatile uint32_t*) (0xE000E010U) = (1U << 0) | (1U << 2);

        // COUNTFLAG 기다림
        while(!(*(volatile uint32_t*) (0xE000E010U) & (1U << 16)))
        {
        }

        // SysTick disable
        *(volatile uint32_t*) (0xE000E010U) = 0U;
    }
}