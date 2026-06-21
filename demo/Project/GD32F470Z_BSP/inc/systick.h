#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>

void systick_config(void);
void delay_ms(uint32_t count);
void delay_decrement(void);

#endif
