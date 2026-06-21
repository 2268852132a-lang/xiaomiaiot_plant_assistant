#ifndef E1_H
#define E1_H

#include "i2c.h"

#define  out

typedef struct
{
   i2c_addr_def rgb_addr[4];
   i2c_addr_def tube_addr[4];
}e1_addr_def;


#define PCA9685_ADDRESS_E1    0xC0			//0xC0,0xC2,0xC4,0xC6
#define HT16K33_ADDRESS_E1    0xE0


#define E1_PCA9685_SUBADR1       0x2
#define E1_PCA9685_SUBADR2       0x3
#define E1_PCA9685_SUBADR3       0x4
#define E1_PCA9685_MODE1         0x0
#define E1_PCA9685_PRESCALE      0xFE

#define LED0_ON_L             0x6
#define LED0_ON_H             0x7
#define LED0_OFF_L            0x8
#define LED0_OFF_H            0x9
#define ALLLED_ON_L           0xFA
#define ALLLED_ON_H           0xFB
#define ALLLED_OFF_L          0xFC
#define ALLLED_OFF_H          0xFD


/* HT16K33  CMD */
#define	SYSTEM_ON				  0x21
#define	SET_INT_NONE		      0xA0
#define	DISPLAY_ON			      0x81
#define	DISPLAY_OFF			      0x80
#define	DIMMING_SET_DEFAULT		  0xEF


#define NODIS                 0x10



i2c_addr_def e1_init(uint8_t address);
void e1_all_init(e1_addr_def *e1_address,uint8_t e1_rgb_addr,uint8_t e1_tube_addr);
void pca9685_e1_init(uint32_t i2c_periph,uint8_t i2c_addr);
void set_e1_pca9685_frequency(uint32_t i2c_periph,uint8_t i2c_addr,uint16_t frequency);
void set_e1_pca9685_pwm(uint32_t i2c_periph,uint8_t i2c_addr,uint8_t num, uint16_t on, uint16_t off);
void e1_rgb_control(uint32_t i2c_periph,uint8_t i2c_addr,uint8_t red,uint8_t green,uint8_t blue);
void ht16k33_e1_init(uint32_t i2c_periph,uint8_t i2c_addr);
void ht16k33_e1_display_data(uint32_t i2c_periph,uint8_t i2c_addr,uint8_t bit,uint8_t data);
void e1_digital_display(uint32_t i2c_periph,uint8_t i2c_addr,uint8_t dis1,uint8_t dis2,uint8_t dis3,uint8_t dis4);


#endif
