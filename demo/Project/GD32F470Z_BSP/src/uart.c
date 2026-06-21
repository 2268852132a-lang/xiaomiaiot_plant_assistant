#include "uart.h"
#include "systick.h"
#include <string.h>

// ============ USART0 接收缓冲区（U2P 文本帧 + C2 二进制帧共用）============
uint8_t uart0_rx_buf[512];
volatile uint16_t uart0_rx_len = 0;

// ============ USART2 接收缓冲区（预留，当前未用）============
uint8_t uart2_rx_buf[512];
volatile uint16_t uart2_rx_len = 0;

// ============ 发送 ============
uint16_t uart_send_bytes(uint32_t periph, uint8_t *data, uint16_t len) {
    for(uint16_t i = 0; i < len; i++) {
        while(usart_flag_get(periph, USART_FLAG_TC) == RESET);
        usart_data_transmit(periph, data[i]);
    }
    while(usart_flag_get(periph, USART_FLAG_TC) == RESET);
    return len;
}

// ============ 接收 ============
uint16_t uart_receive_bytes(uint32_t periph, uint8_t *data, uint16_t len, uint16_t timeout) {
    volatile uint16_t *rx_len_ptr = NULL;
    uint8_t *rx_buf_ptr = NULL;

    if(periph == USART0) {
        rx_len_ptr = &uart0_rx_len;
        rx_buf_ptr = uart0_rx_buf;
    } else if(periph == USART2) {
        rx_len_ptr = &uart2_rx_len;
        rx_buf_ptr = uart2_rx_buf;
    } else {
        return 0;
    }

    // 等待数据到达
    uint16_t waited = 0;
    while(*rx_len_ptr == 0 && waited < timeout) {
        delay_ms(1);
        waited++;
    }
    if(*rx_len_ptr == 0) return 0;

    // 等空闲：连续 50ms 无新数据则认为传输完成
    uint16_t last_len = *rx_len_ptr;
    uint8_t idle = 0;
    while(idle < 50 && waited < timeout) {
        delay_ms(1);
        waited++;
        if(*rx_len_ptr != last_len) {
            last_len = *rx_len_ptr;
            idle = 0;
        } else {
            idle++;
        }
    }

    // 读出数据
    usart_interrupt_disable(periph, USART_INT_RBNE);
    uint16_t n = (*rx_len_ptr < len) ? *rx_len_ptr : len;
    memcpy(data, rx_buf_ptr, n);
    *rx_len_ptr = 0;
    usart_interrupt_enable(periph, USART_INT_RBNE);
    return n;
}

// ============ 初始化 ============
void uart_init(uint32_t periph) {
    if(periph == USART0) {
        nvic_irq_enable(USART0_IRQn, 0, 0);
        rcu_periph_clock_enable(RCU_USART0);
        rcu_periph_clock_enable(RCU_GPIOA);
        gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_9);
        gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_10);
        gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_9);
        gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9);
        gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_10);
        gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_10);
        usart_deinit(USART0);
        usart_baudrate_set(USART0, 115200);
        usart_receive_config(USART0, USART_RECEIVE_ENABLE);
        usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
        usart_enable(USART0);
        usart_interrupt_enable(USART0, USART_INT_RBNE);

    } else if(periph == USART2) {
        nvic_irq_enable(USART2_IRQn, 0, 0);
        rcu_periph_clock_enable(RCU_USART2);
        rcu_periph_clock_enable(RCU_GPIOC);
        gpio_af_set(GPIOC, GPIO_AF_7, GPIO_PIN_10);
        gpio_af_set(GPIOC, GPIO_AF_7, GPIO_PIN_11);
        gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_10);
        gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_10);
        gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_11);
        gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_11);
        usart_deinit(USART2);
        usart_baudrate_set(USART2, 115200);
        usart_receive_config(USART2, USART_RECEIVE_ENABLE);
        usart_transmit_config(USART2, USART_TRANSMIT_ENABLE);
        usart_enable(USART2);
        usart_interrupt_enable(USART2, USART_INT_RBNE);
    }
}

// ============ 中断服务 ============

// USART0 ISR — 原始字节进 uart0_rx_buf（供 C2 初始化轮询/check_c2_plant_data 使用）
//             同时 U2P 文本帧在 main.c parse_u2p_data 中从 uart0_rx_buf 解析
void USART0_IRQHandler(void) {
    if(usart_interrupt_flag_get(USART0, USART_INT_FLAG_RBNE) != RESET) {
        usart_interrupt_flag_clear(USART0, USART_INT_FLAG_RBNE);
        uint8_t ch = usart_data_receive(USART0);
        if(uart0_rx_len < sizeof(uart0_rx_buf)) {
            uart0_rx_buf[uart0_rx_len++] = ch;
        }
    }
}

// USART2 ISR（预留，当前未用）
void USART2_IRQHandler(void) {
    if(usart_interrupt_flag_get(USART2, USART_INT_FLAG_RBNE) != RESET) {
        usart_interrupt_flag_clear(USART2, USART_INT_FLAG_RBNE);
        if(uart2_rx_len < sizeof(uart2_rx_buf)) {
            uart2_rx_buf[uart2_rx_len++] = usart_data_receive(USART2);
        } else {
            usart_data_receive(USART2);
        }
    }
}
