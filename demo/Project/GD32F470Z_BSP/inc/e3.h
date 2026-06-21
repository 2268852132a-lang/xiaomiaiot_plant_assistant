#ifndef E3_H
#define E3_H

#include "i2c.h"
#define  out

typedef struct
{
   i2c_addr_def curtain_addr[4];
}e3_addr_def;


#define CURTAIN_ADDRESS_E3                  0x38




i2c_addr_def e3_init(uint8_t address);
void e3_all_init(e3_addr_def *e3_address,uint8_t e3_addr);
void e3_set_position(uint32_t i2c_periph,uint8_t i2c_addr,uint8_t re_pos);



#endif
