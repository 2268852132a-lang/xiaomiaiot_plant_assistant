#ifndef E2_H
#define E2_H

#include "i2c.h"

#define  out 
#define  PCA9685_ADDRESS_E2      0xC8			//0xC8¡¢0xCA¡¢0xCC¡¢0xCE

#define  E2_PCA9685_SUBADR1         0x2
#define  E2_PCA9685_SUBADR2         0x3
#define  E2_PCA9685_SUBADR3         0x4
#define  E2_PCA9685_MODE1           0x0
#define  E2_PCA9685_PRESCALE        0xFE

#define  CN0_ON_L               0x6
#define  CN0_ON_H               0x7
#define  CN0_OFF_L              0x8
#define  CN0_OFF_H              0x9

#define  ALLCN_ON_L             0xFA
#define  ALLCN_ON_H             0xFB
#define  ALLCN_OFF_L            0xFC
#define  ALLCN_OFF_H            0xFD


typedef struct
{
   i2c_addr_def motor_addr[4];
}e2_addr_def;

i2c_addr_def e2_init(uint8_t address);
void pca9685_e2_init(uint32_t i2c_periph,uint8_t i2c_addr);
void e2_all_init(e2_addr_def *e2_address,uint8_t e2_addr);
void e2_speed_control(uint32_t i2c_periph,uint8_t i2c_addr,uint8_t speed_level);

#endif



