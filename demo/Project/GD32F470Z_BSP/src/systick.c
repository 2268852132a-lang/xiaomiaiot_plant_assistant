#include "gd32f4xx.h"
#include "systick.h"

static __IO uint32_t delay;
volatile uint32_t tick_ms = 0;
static volatile uint16_t time_count = 0;
extern volatile uint8_t second_flag;

void systick_config(void) {
    /* setup systick timer for 1000Hz interrupts */
    if (SysTick_Config(SystemCoreClock / 1000U)) {
        /* capture error */
        while (1);
    }
    /* configure the systick handler priority */
    NVIC_SetPriority(SysTick_IRQn, 0x00U);
}

void delay_ms(uint32_t count) {
    delay = count;
    while (0U != delay);
}

void delay_decrement(void) {
    if (0U != delay) {
        delay--;
    }
}

void SysTick_Handler(void) {
    delay_decrement();
    tick_ms++;
    if(time_count++ >= 700) {
        time_count = 0;
        second_flag = 1;
    }
}