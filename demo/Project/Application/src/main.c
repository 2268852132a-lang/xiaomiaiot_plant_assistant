#include "gd32f4xx.h"
#include "gd32f470z_eval.h"
#include "s1.h"
#include "e1.h"
#include "e2.h"
#include "e3.h"
#include "i2c.h"
#include "plant_logic.h"
#include "uart.h"
#include "systick.h"
#include "c2.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// ==================== 全局变量 ====================
volatile uint8_t second_flag = 0;
static uint8_t rgb_mode = 0; // 0 = 跟随评分，1-4 = 固定颜色

// I2C 设备地址结构体
i2c_addr_def s1_board;
i2c_addr_def e1_tube_board;
i2c_addr_def e1_rgb_board;
i2c_addr_def e2_fan_board;
i2c_addr_def e3_curtain_board;
i2c_addr_def s2_sht35_board;
i2c_addr_def s2_light_board;

// UART 接收缓冲区（USART0 ISR 写入 uart0_rx_buf，由 parse_u2p_data 和 check_c2_plant_data 读取）
uint8_t print_buffer[100];

// C2 Zigbee 状态
static uint8_t c2_ok = 0;

// ==================== 函数声明 ====================
void hardware_self_test(void);
void update_sensors(void);
void handle_key(uint8_t key);
void parse_u2p_data(void);
void check_c2_plant_data(void);
void process_uart0_rx(void);
uint16_t uart_print(uint32_t usart_periph, uint8_t* data, uint16_t len);
void debug_printf(uint32_t usart_periph, char* string);
void update_rgb_indicator(void);
void blink_rgb(uint8_t r, uint8_t g, uint8_t b, uint8_t times);
void set_fixed_rgb(uint8_t mode);

// ==================== 主函数 ====================
int main(void) {
    nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);
    gd_eval_com_init(EVAL_COM0);
    uart_init(USART0); // USART0: 调试口 + C2 Zigbee（物理连接主板槽位1）
    systick_config();
    init_i2c();
    plant_logic_init();

    sprintf((char*)print_buffer, "\r\n========================================\r\n");
    debug_printf(EVAL_COM0, (char*)print_buffer);
    sprintf((char*)print_buffer, "   Plant Assistant v2.1 (C2 fix)\r\n");
    debug_printf(EVAL_COM0, (char*)print_buffer);
    sprintf((char*)print_buffer, "========================================\r\n\r\n");
    debug_printf(EVAL_COM0, (char*)print_buffer);

    hardware_self_test();

    // C2 Zigbee 初始化（仿 StudyGuard: 保持中断启用，c2_init 从 ISR 缓冲区读响应）
    if(c2_init(TERMINAL)) {
        debug_printf(EVAL_COM0, "[C2] Terminal OK\r\n");
        c2_ok = 1;
        // RGB 闪绿 3 次提示 C2 初始化成功
        blink_rgb(0, 255, 0, 3);
        // 发测试广播确认无线通
        c2_broadcast_data("TEST|U1P_C2_OK", 0x01);
        debug_printf(EVAL_COM0, "[C2] Sent TEST broadcast\r\n");
    } else {
        debug_printf(EVAL_COM0, "[C2] Init failed\r\n");
        // RGB 闪红 5 次警告 C2 初始化失败
        blink_rgb(255, 0, 0, 5);
    }

    // 初始化数码管显示
    if(e1_tube_board.flag) {
        e1_digital_display(e1_tube_board.periph, e1_tube_board.addr, 0, 0, 0, 0);
    } else {
        // 数码管没找到也强行试一次（可能是 I2C 半锁死）
        e1_digital_display(I2C0, HT16K33_ADDRESS_E1, 0, 0, 0, 0);
    }

    // 初始化 RGB LED 为绿色（初始状态）
    if(e1_rgb_board.flag) {
        e1_rgb_control(e1_rgb_board.periph, e1_rgb_board.addr, 0, 255, 0);
    }

    uint8_t last_key = SWN;
    uint8_t key;

    while(1) {
        // 统一处理 USART0 接收（C2 帧 + U2P 文本帧共用 uart0_rx_buf）
        process_uart0_rx();

        // 按键扫描
        if(s1_board.flag) {
            key = s1_key_scan(s1_board.periph, s1_board.addr);
            // 调试：如果有按键按下，打印出来
            if(key != SWN) {
                sprintf((char*)print_buffer, "[KEY] Raw key: %d\n", key);
                debug_printf(EVAL_COM0, (char*)print_buffer);
            }
            if(key != SWN && key != last_key) {
                delay_ms(20);
                if(s1_key_scan(s1_board.periph, s1_board.addr) == key) {
                    handle_key(key);
                    while(s1_key_scan(s1_board.periph, s1_board.addr) == key);
                }
            }
            last_key = key;
        }

        // 每秒更新传感器和显示
        if(second_flag) {
            second_flag = 0;
            update_sensors();
            calc_score();
            update_evaporation();
            check_water_alert();

            // 自动控制逻辑
            auto_curtain();
            auto_fan();

            // 浇水提醒：通过 C2 Zigbee 广播给 U2P（U2P 走 Server酱 推送微信）
            if(current_env.water_alert && c2_ok) {
                sprintf((char*)print_buffer, "WATER|%s|%.1f|%.1f|%.0f",
                    current_profile.name,
                    current_env.temperature,
                    current_env.humidity,
                    current_env.light);
                c2_broadcast_data((char*)print_buffer, 0x01);
                debug_printf(EVAL_COM0, "[C2] WATER alert broadcast\r\n");
            }

            // 更新 RGB 指示灯（如果不是固定颜色模式）
            if(rgb_mode == 0) {
                update_rgb_indicator();
            }

            // 刷新数码管显示评分（基准分+55）
            if(e1_tube_board.flag) {
                int display_val = current_env.score + 55;
                if(display_val > 99) display_val = 99;
                if(display_val < 0) display_val = 0;
                e1_digital_display(e1_tube_board.periph, e1_tube_board.addr,
                                   display_val / 10, display_val % 10, 0x10, 0x10);
            }

            sprintf((char*)print_buffer, "[%s] T=%.1f H=%.1f L=%.0f Score=%d\r\n",
                    current_profile.name,
                    current_env.temperature,
                    current_env.humidity,
                    current_env.light,
                    current_env.score);
            debug_printf(EVAL_COM0, (char*)print_buffer);
        }

        delay_ms(10);
    }
}

void set_fixed_rgb(uint8_t mode) {
    if(!e1_rgb_board.flag) return;
    
    uint8_t r = 0, g = 0, b = 0;
    
    switch(mode) {
        case 1: // 红色
            r = 255;
            break;
        case 2: // 绿色
            g = 255;
            break;
        case 3: // 蓝色
            b = 255;
            break;
        case 4: // 紫色
            r = 255;
            b = 255;
            break;
        default:
            update_rgb_indicator();
            return;
    }
    
    e1_rgb_control(e1_rgb_board.periph, e1_rgb_board.addr, r, g, b);
}

void update_rgb_indicator(void) {
    if(!e1_rgb_board.flag || rgb_mode != 0) {
        return;
    }

    uint8_t r = 0, g = 0, b = 0;

    int display_val = current_env.score + 55;
    if(display_val > 99) display_val = 99;
    if(display_val < 0) display_val = 0;

    if(display_val >= 90) {
        g = 255;
    } else if(display_val >= 70) {
        r = 255; g = 255;
    } else if(display_val >= 50) {
        r = 255; g = 165;
    } else {
        r = 255;
    }

    e1_rgb_control(e1_rgb_board.periph, e1_rgb_board.addr, r, g, b);
}

void blink_rgb(uint8_t r, uint8_t g, uint8_t b, uint8_t times) {
    if(!e1_rgb_board.flag) {
        return;
    }

    for(uint8_t i = 0; i < times; i++) {
        e1_rgb_control(e1_rgb_board.periph, e1_rgb_board.addr, r, g, b);
        delay_ms(200);
        e1_rgb_control(e1_rgb_board.periph, e1_rgb_board.addr, 0, 0, 0);
        delay_ms(200);
    }
}

void hardware_self_test(void)
{
    sprintf((char*)print_buffer, "--- Hardware Self Test ---\r\n");
    debug_printf(EVAL_COM0, (char*)print_buffer);

    // 只做定向初始化，不做全地址扫描
    e1_tube_board = e1_init(HT16K33_ADDRESS_E1);
    e1_rgb_board = e1_init(PCA9685_ADDRESS_E1);
    e2_fan_board = e2_init(PCA9685_ADDRESS_E2);
    if(e1_rgb_board.flag) e1_rgb_control(e1_rgb_board.periph, e1_rgb_board.addr, 0, 0, 0);

    e3_curtain_board = e3_init(CURTAIN_ADDRESS_E3);

    // S2 传感器：扩大扫描范围，覆盖拨码开关两种配置
    s2_sht35_board = scan_board_range(ADDR_S2_SHT35, 8);
    s2_light_board = scan_board_range(ADDR_S2_BH1750, 8);
    // BH1750 备用地址 0xB8 (ADDR=HIGH) 也扫一下
    if(!s2_light_board.flag) {
        s2_light_board = scan_board_range(0xB0, 8);
    }
    if(s2_light_board.flag) {
        i2c_cmd_write(s2_light_board.periph, s2_light_board.addr, 0x01);
        i2c_cmd_write(s2_light_board.periph, s2_light_board.addr, 0x07);
    }

    // S1 按键：标准范围
    s1_board = s1_init(HT16K33_ADDRESS_S1);
    if(s1_board.flag) {
        i2c_cmd_write(s1_board.periph, s1_board.addr, 0x21);
        i2c_cmd_write(s1_board.periph, s1_board.addr, 0xA0);
    }

    // 串口输出
    sprintf((char*)print_buffer,
        "S1:%s | Tube:%s | RGB:%s | Fan:%s | Cur:%s | T/H:%s | Light:%s\r\n",
        s1_board.flag?"OK":"--", e1_tube_board.flag?"OK":"--",
        e1_rgb_board.flag?"OK":"--", e2_fan_board.flag?"OK":"--",
        e3_curtain_board.flag?"OK":"--",
        s2_sht35_board.flag?"OK":"--", s2_light_board.flag?"OK":"--");
    debug_printf(EVAL_COM0, (char*)print_buffer);
    sprintf((char*)print_buffer, "--- Self Test Done ---\r\n\r\n");
    debug_printf(EVAL_COM0, (char*)print_buffer);
}

void update_sensors(void)
{
    if(s2_sht35_board.flag) {
        uint8_t data[6];
        i2c_cmd_write(s2_sht35_board.periph, s2_sht35_board.addr, 0x24);
        i2c_cmd_write(s2_sht35_board.periph, s2_sht35_board.addr, 0x0B);
        delay_ms(20);
        if(i2c_read(s2_sht35_board.periph, s2_sht35_board.addr, 0x00, data, 6)) {
            uint16_t st = (data[0] << 8) | data[1];
            uint16_t srh = (data[3] << 8) | data[4];
            current_env.temperature = -45.0f + 175.0f * (float)st / 65535.0f;
            current_env.humidity = 100.0f * (float)srh / 65535.0f;
        }
    }

    if(s2_light_board.flag) {
        uint8_t data[2];
        i2c_cmd_write(s2_light_board.periph, s2_light_board.addr, 0x10);
        delay_ms(180);
        if(i2c_read_no_reg(s2_light_board.periph, s2_light_board.addr, data, 2)) {
            uint16_t raw_light = (data[0] << 8) | data[1];
            current_env.light = (float)raw_light / 1.2f;
        }
    }
}

void handle_key(uint8_t key)
{
    static uint8_t curtain_pos = 0;
    switch(key) {
        case SW1:
            // 切换 RGB 颜色模式
            rgb_mode = (rgb_mode + 1) % 5; // 0-4 循环
            if(rgb_mode == 0) {
                // 回到跟随评分模式
                update_rgb_indicator();
                sprintf((char*)print_buffer, "RGB Mode: Auto\r\n");
            } else {
                set_fixed_rgb(rgb_mode);
                sprintf((char*)print_buffer, "RGB Mode: %d\r\n", rgb_mode);
            }
            debug_printf(EVAL_COM0, (char*)print_buffer);
            break;
        case SW2:
            switch_plant();
            
            // 显示植物编号 (1-3)
            if(e1_tube_board.flag) {
                e1_digital_display(e1_tube_board.periph, e1_tube_board.addr,
                                   (uint8_t)(current_plant_type + 1), 0x10, 0x10, 0x10);
            }
            
            // RGB 闪烁两次绿色
            blink_rgb(0, 255, 0, 2);
            
            // 如果是固定颜色模式，恢复到固定颜色
            if(rgb_mode != 0) {
                set_fixed_rgb(rgb_mode);
            }
            
            sprintf((char*)print_buffer, "Plant Switched: %s\r\n", current_profile.name);
            debug_printf(EVAL_COM0, (char*)print_buffer);
            break;
        case SW3:
            curtain_pos = (curtain_pos == 0) ? 100 : 0;
            if(e3_curtain_board.flag) {
                e3_set_position(e3_curtain_board.periph, e3_curtain_board.addr, curtain_pos);
            }
            sprintf((char*)print_buffer, "Curtain: %d%%\r\n", curtain_pos);
            debug_printf(EVAL_COM0, (char*)print_buffer);
            break;
        default:
            sprintf((char*)print_buffer, "Other Key\r\n");
            debug_printf(EVAL_COM0, (char*)print_buffer);
            break;
    }
}

// 统一 USART0 接收调度：C2 帧 (0xFC) + U2P 文本帧 (换行分隔)
void process_uart0_rx(void)
{
    if(uart0_rx_len == 0) return;

    __disable_irq();
    uint16_t n = uart0_rx_len;
    uint8_t local[256];
    uint16_t copy_len = (n > 255) ? 255 : n;
    memcpy(local, uart0_rx_buf, copy_len);
    uart0_rx_len = 0;
    __enable_irq();

    // 遍历所有字节，按帧头分发
    uint16_t i = 0;
    while(i < copy_len) {
        // --- C2 二进制帧: 0xFC, len, 0x01, mode, data... ---
        if(local[i] == 0xFC && c2_ok && i + 4 < copy_len) {
            uint8_t data_len = local[i + 1];
            if(data_len >= 2 && i + 4 + data_len - 2 <= copy_len) {
                uint8_t* p = &local[i + 4];
                uint8_t plen = data_len - 2;
                uint8_t sep_pos = 0xFF;
                for(uint8_t j = 0; j < plen; j++) {
                    if(p[j] == '|') { sep_pos = j; break; }
                }
                if(sep_pos != 0xFF) {
                    p[sep_pos] = '\0';
                    char* name = (char*)p;
                    char* score_str = (char*)&p[sep_pos + 1];
                    if(*score_str >= '0' && *score_str <= '9') {
                        float score = (float)atof(score_str);
                        sprintf((char*)print_buffer, "[C2] Plant: %s %.2f\r\n", name, score);
                        debug_printf(EVAL_COM0, (char*)print_buffer);
                        u1p_fusion_decision(name, score);
                        blink_rgb(0, 0, 255, 1);
                    }
                }
                i += 4 + data_len - 2;
                continue;
            }
        }

        // --- 文本帧: 找 \n 或 \r ---
        uint16_t line_start = i;
        while(i < copy_len && local[i] != '\n' && local[i] != '\r') i++;
        if(i > line_start) {
            local[i] = '\0';
            char* line = (char*)&local[line_start];

            // 跳过调试输出 (serialprint / [C2] / [RECV] 等自动忽略)
            char* sep = strchr(line, '|');
            if(sep && line[0] >= ' ') {
                *sep = '\0';
                char* name = line;
                char* score_str = sep + 1;
                if((*score_str >= '0' && *score_str <= '9') || *score_str == '.') {
                    float score = (float)atof(score_str);
                    sprintf((char*)print_buffer, "[RECV] U2P: %s, %.2f\r\n", name, score);
                    debug_printf(EVAL_COM0, (char*)print_buffer);

                    plant_type_t old = current_plant_type;
                    u1p_fusion_decision(name, score);
                    blink_rgb(0, 0, 255, 1);

                    if(old != current_plant_type) {
                        sprintf((char*)print_buffer, "[U2P] Switched to: %s\r\n", current_profile.name);
                        debug_printf(EVAL_COM0, (char*)print_buffer);
                        if(e1_tube_board.flag) {
                            e1_digital_display(e1_tube_board.periph, e1_tube_board.addr,
                                               (uint8_t)(current_plant_type + 1), 0x10, 0x10, 0x10);
                        }
                        blink_rgb(0, 255, 0, 2);
                        if(rgb_mode != 0) set_fixed_rgb(rgb_mode);
                    }

                    // ACK
                    usart_data_transmit(USART0, 'A');
                    while(RESET == usart_flag_get(USART0, USART_FLAG_TC));
                    usart_data_transmit(USART0, 'C');
                    while(RESET == usart_flag_get(USART0, USART_FLAG_TC));
                    usart_data_transmit(USART0, 'K');
                    while(RESET == usart_flag_get(USART0, USART_FLAG_TC));
                    usart_data_transmit(USART0, '\n');
                    while(RESET == usart_flag_get(USART0, USART_FLAG_TC));
                }
            }
        }
        // 跳过 \n \r
        while(i < copy_len && (local[i] == '\n' || local[i] == '\r')) i++;
    }
}

uint16_t uart_print(uint32_t usart_periph, uint8_t* data, uint16_t len)
{
    uint8_t i;
    for(i = 0; i < len; i++) {
        while(usart_flag_get(usart_periph, USART_FLAG_TC) == RESET);
        usart_data_transmit(usart_periph, data[i]);
    }
    while(usart_flag_get(usart_periph, USART_FLAG_TC) == RESET);
    return len;
}

void debug_printf(uint32_t usart_periph, char* string)
{
    uint16_t len;
    len = strlen(string);
    uart_print(usart_periph, (uint8_t*)string, len);
}
