#ifndef PLANT_LOGIC_H
#define PLANT_LOGIC_H

#include <stdint.h>

// 植物类型
typedef enum {
    PLANT_GREEN_ROLO,
    PLANT_SUCCULENT,
    PLANT_MONEY_TREE
} plant_type_t;

// 植物档案
typedef struct {
    char name[16];
    float temp_min;
    float temp_max;
    float humi_min;
    float humi_max;
    float light_min;
    float light_max;
    uint16_t curtain_close_threshold;
    uint16_t curtain_open_threshold;
    uint16_t fan_open_threshold;
    uint16_t fan_close_threshold;
} plant_profile_t;

// 环境数据
typedef struct {
    float temperature;
    float humidity;
    float light;
    int score;
    float evaporation_index;
    float evaporation_sum;
    uint8_t water_alert;
    uint16_t water_alert_cooldown;
} env_data_t;

// 全局变量（外部可访问）
extern plant_profile_t current_profile;
extern env_data_t current_env;
extern plant_type_t current_plant_type;

// 外部声明硬件地址（来自 main.c）
#include "i2c.h"
extern i2c_addr_def e2_fan_board;
extern i2c_addr_def e3_curtain_board;

// 函数声明
void plant_logic_init(void);
void switch_plant(void);
void update_sensors(void);
void calc_score(void);
void update_evaporation(void);
void check_water_alert(void);
void auto_curtain(void);
void auto_fan(void);
void u1p_fusion_decision(char* name, float score);

#endif