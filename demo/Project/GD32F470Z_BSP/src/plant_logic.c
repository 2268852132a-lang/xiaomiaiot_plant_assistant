#include "plant_logic.h"
#include "i2c.h"
#include <string.h>
#include <math.h>

// 植物档案定义
static const plant_profile_t profiles[] = {
    {"绿萝",   18.0f, 28.0f, 50.0f, 70.0f, 200.0f, 500.0f, 800, 400, 80, 70},
    {"多肉",   15.0f, 25.0f, 30.0f, 50.0f, 500.0f, 1000.0f, 1200, 600, 70, 60},
    {"发财树", 20.0f, 30.0f, 40.0f, 60.0f, 300.0f, 600.0f, 800, 400, 80, 70}
};

// 全局变量定义
plant_profile_t current_profile;
env_data_t current_env;
plant_type_t current_plant_type = PLANT_GREEN_ROLO;

void plant_logic_init(void) {
    current_profile = profiles[current_plant_type];
    current_env.temperature = 25.0f;
    current_env.humidity = 50.0f;
    current_env.light = 300.0f;
    current_env.score = 0;
    current_env.evaporation_sum = 0;
    current_env.water_alert = 0;
    current_env.water_alert_cooldown = 0;
}

void switch_plant(void) {
    current_plant_type = (current_plant_type + 1) % 3;
    current_profile = profiles[current_plant_type];
}

void calc_score(void) {
    float temp_score = 100.0f;
    float humi_score = 100.0f;
    float light_score = 100.0f;

    if (current_env.temperature < current_profile.temp_min) {
        temp_score -= (current_profile.temp_min - current_env.temperature) * 10.0f;
    } else if (current_env.temperature > current_profile.temp_max) {
        temp_score -= (current_env.temperature - current_profile.temp_max) * 10.0f;
    }

    if (current_env.humidity < current_profile.humi_min) {
        humi_score -= ((current_profile.humi_min - current_env.humidity) / 5.0f) * 2.0f;
    } else if (current_env.humidity > current_profile.humi_max) {
        humi_score -= ((current_env.humidity - current_profile.humi_max) / 5.0f) * 2.0f;
    }

    if (current_env.light < current_profile.light_min) {
        light_score -= ((current_profile.light_min - current_env.light) / 50.0f) * 0.5f;
    } else if (current_env.light > current_profile.light_max) {
        light_score -= ((current_env.light - current_profile.light_max) / 50.0f) * 0.5f;
    }

    if (temp_score < 0) temp_score = 0;
    if (humi_score < 0) humi_score = 0;
    if (light_score < 0) light_score = 0;

    current_env.score = (int)(temp_score * 0.3f + humi_score * 0.4f + light_score * 0.3f);
    if (current_env.score > 99) current_env.score = 99;
}

void update_evaporation(void) {
    float evap = current_env.temperature * 0.3f + current_env.light * 0.5f - current_env.humidity * 0.2f;
    
    if (current_env.water_alert_cooldown == 0) {
        current_env.evaporation_sum += evap;
    } else {
        current_env.water_alert_cooldown--;
    }
}

void check_water_alert(void) {
    if (current_env.evaporation_sum > 500.0f) {
        current_env.water_alert = 1;
        current_env.evaporation_sum = 0;
        current_env.water_alert_cooldown = 3600;
    } else {
        current_env.water_alert = 0;
    }
}

void auto_curtain(void) {
    static uint8_t curtain_state = 0;
    if (current_env.light > current_profile.curtain_close_threshold) {
        if (curtain_state != 1) {
            if (e3_curtain_board.flag) {
                e3_set_position(e3_curtain_board.periph, e3_curtain_board.addr, 100);
            }
            curtain_state = 1;
        }
    } else if (current_env.light < current_profile.curtain_open_threshold) {
        if (curtain_state != 0) {
            if (e3_curtain_board.flag) {
                e3_set_position(e3_curtain_board.periph, e3_curtain_board.addr, 0);
            }
            curtain_state = 0;
        }
    }
}

void auto_fan(void) {
    static uint8_t fan_state = 0;
    if (current_env.humidity > current_profile.fan_open_threshold) {
        if (fan_state != 1) {
            if (e2_fan_board.flag) {
                e2_speed_control(e2_fan_board.periph, e2_fan_board.addr, 90);
            }
            fan_state = 1;
        }
    } else if (current_env.humidity < current_profile.fan_close_threshold) {
        if (fan_state != 0) {
            if (e2_fan_board.flag) {
                e2_speed_control(e2_fan_board.periph, e2_fan_board.addr, 0);
            }
            fan_state = 0;
        }
    }
}

void u1p_fusion_decision(char* name, float score) {
    if (score < 0.6f) return; // 只有置信度高于 60% 才切换
    
    // 根据识别出的名字自动切换档案
    if (strstr(name, "绿萝")) {
        current_plant_type = PLANT_GREEN_ROLO;
    } else if (strstr(name, "多肉")) {
        current_plant_type = PLANT_SUCCULENT;
    } else if (strstr(name, "发财树")) {
        current_plant_type = PLANT_MONEY_TREE;
    }
    
    current_profile = profiles[current_plant_type];
}