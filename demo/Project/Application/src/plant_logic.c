#include "../inc/plant_logic.h"
#include "e2.h"
#include "e3.h"
#include <string.h>
#include <math.h>

// 植物档案定义
static const plant_profile_t profiles[] = {
    {"绿萝", 18.0f, 28.0f, 50.0f, 70.0f, 200.0f, 500.0f, 800.0f, 400.0f, 60, 50},
    {"多肉", 15.0f, 25.0f, 30.0f, 50.0f, 500.0f, 1000.0f, 1200.0f, 600.0f, 55, 45},
    {"发财树", 20.0f, 30.0f, 40.0f, 60.0f, 300.0f, 600.0f, 800.0f, 400.0f, 60, 50}
};

plant_profile_t current_profile;
env_data_t current_env;
plant_type_t current_plant_type = PLANT_GREEN_ROLO;

void plant_logic_init(void) {
    current_profile = profiles[current_plant_type];
    memset(&current_env, 0, sizeof(env_data_t));
}

void switch_plant(void) {
    current_plant_type = (current_plant_type + 1) % 3;
    current_profile = profiles[current_plant_type];
}

void calc_score(void) {
    float temp_score = 100.0f;
    float humi_score = 100.0f;
    float light_score = 100.0f;

    // 温度评分
    if (current_env.temperature < current_profile.temp_min) {
        temp_score -= (current_profile.temp_min - current_env.temperature) * 10.0f;
    } else if (current_env.temperature > current_profile.temp_max) {
        temp_score -= (current_env.temperature - current_profile.temp_max) * 10.0f;
    }

    // 湿度评分
    if (current_env.humidity < current_profile.humi_min) {
        humi_score -= (current_profile.humi_min - current_env.humidity) * 2.0f;
    } else if (current_env.humidity > current_profile.humi_max) {
        humi_score -= (current_env.humidity - current_profile.humi_max) * 2.0f;
    }

    // 光照评分
    if (current_env.light < current_profile.light_min) {
        light_score -= (current_profile.light_min - current_env.light) / 2.0f;
    } else if (current_env.light > current_profile.light_max) {
        light_score -= (current_env.light - current_profile.light_max) / 2.0f;
    }

    // 限制最低分
    if (temp_score < 0) temp_score = 0;
    if (humi_score < 0) humi_score = 0;
    if (light_score < 0) light_score = 0;

    current_env.score = (int)(temp_score * 0.3f + humi_score * 0.4f + light_score * 0.3f);
    if (current_env.score > 99) current_env.score = 99;
}

void update_evaporation(void) {
    current_env.evaporation_index = current_env.temperature * 0.3f + 
                                    current_env.light * 0.5f - 
                                    current_env.humidity * 0.2f;
    
    if (current_env.water_alert_cooldown == 0) {
        current_env.evaporation_sum += current_env.evaporation_index;
    } else {
        current_env.water_alert_cooldown--;
    }
}

void check_water_alert(void) {
    if (current_env.evaporation_sum > 500.0f) {
        current_env.water_alert = 1;
        current_env.evaporation_sum = 0;
        current_env.water_alert_cooldown = 3600; // 1小时冷却期 (假设每秒调用一次)
    } else {
        current_env.water_alert = 0;
    }
}

void auto_curtain(void) {
    static uint8_t curtain_position = 0;
    
    uint32_t periph = e3_curtain_board.flag ? e3_curtain_board.periph : I2C0;
    uint8_t addr   = e3_curtain_board.flag ? e3_curtain_board.addr   : CURTAIN_ADDRESS_E3;
    
    if (current_env.light > current_profile.curtain_close_threshold) {
        curtain_position = 100;
        e3_set_position(periph, addr, curtain_position);
    } else if (current_env.light < current_profile.curtain_open_threshold) {
        curtain_position = 0;
        e3_set_position(periph, addr, curtain_position);
    }
}

void auto_fan(void) {
    static uint8_t fan_speed = 0;
    
    uint32_t periph = e2_fan_board.flag ? e2_fan_board.periph : I2C0;
    uint8_t addr   = e2_fan_board.flag ? e2_fan_board.addr   : PCA9685_ADDRESS_E2;
    
    if (current_env.humidity > current_profile.fan_open_threshold) {
        fan_speed = 90;
        e2_speed_control(periph, addr, fan_speed);
    } else if (current_env.humidity < current_profile.fan_close_threshold) {
        fan_speed = 0;
        e2_speed_control(periph, addr, fan_speed);
    }
}

// 视觉融合决策
void u1p_fusion_decision(char* name, float score) {
    if (score < 0.5f) return; // 置信度过低

    // --- 根据识别结果切换植物档案 ---
    if (strstr(name, "绿萝")) {
        current_plant_type = PLANT_GREEN_ROLO;
    } else if (strstr(name, "多肉")) {
        current_plant_type = PLANT_SUCCULENT;
    } else if (strstr(name, "发财树")) {
        current_plant_type = PLANT_MONEY_TREE;
    } else {
        return; // 非目标植物，不切换
    }
    current_profile = profiles[current_plant_type];

    // --- 高置信度下融合控制 (score > 0.7) ---
    if (score > 0.7f) {
        int is_shade_loving = (current_plant_type == PLANT_GREEN_ROLO || current_plant_type == PLANT_MONEY_TREE);

        if (is_shade_loving && current_env.light > 800.0f) {
            // 喜阴植物 + 光照>800lux -> 关窗帘
            if (e3_curtain_board.flag) {
                e3_set_position(e3_curtain_board.periph, e3_curtain_board.addr, 100);
            }
        }
        // 多肉喜光，光照强也不动作
    }
}
