#ifndef I2C_H
#define I2C_H

#include "gd32f4xx.h"

// I2C 设备地址定义 (8位地址)
#define ADDR_S1_KEY        0xE8 // 矩阵按键 (HT16K33)
#define ADDR_S2_SHT35      0x88 // 温湿度 (SHT35)
#define ADDR_S2_BH1750     0x46 // 光照 (BH1750)
#define ADDR_E1_RGB        0xC0 // RGB灯 (PCA9685)
#define ADDR_E1_NIXIE      0xE0 // 数码管 (HT16K33)
#define ADDR_E2_FAN        0xC8 // 风扇 (PCA9685)
#define ADDR_E3_CURTAIN    0x38 // 窗帘 (步进电机控制器)

#define I2C0_SPEED         100000
#define I2C0_SLAVE_ADDRESS7 0x72
#define I2C1_SPEED         100000
#define I2C1_SLAVE_ADDRESS7 0x72

typedef struct {
    uint32_t periph;
    uint8_t addr;
    uint8_t flag;
} i2c_addr_def;

void init_i2c(void);
i2c_addr_def get_board_address(uint8_t address);
i2c_addr_def scan_board_range(uint8_t start_addr, uint8_t count);
uint8_t i2c_addr_poll(uint32_t i2c_periph, uint8_t poll_addr);

void i2c_cmd_write(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t cmd);
void i2c_byte_write(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t write_address, uint8_t buffer);
void i2c_write(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t write_address, uint8_t* p_buffer, uint8_t number_of_byte);
uint8_t i2c_read(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t read_address, uint8_t* p_buffer, uint16_t number_of_byte);
uint8_t i2c_read_no_reg(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t* p_buffer, uint16_t number_of_byte);

void i2c_delay(uint32_t time);
void i2c_delay_byte_write(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t write_address, uint8_t buffer);
void i2c_delay_write(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t write_address, uint8_t* p_buffer, uint8_t number_of_byte);

#endif
