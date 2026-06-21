#ifndef UART_H
#define UART_H

#include "gd32f4xx.h"

// 串口接收状态
typedef enum {
    STATE_RX_IDLE,
    STATE_RX_RECEIVING,
    STATE_RX_ERROR,
    STATE_RX_RECEIVED
} UartRecvState_t;

// 函数声明
void uart_init(uint32_t usart_periph);
void uart_config(uint32_t usart_periph, uint32_t baudrate);
uint16_t uart_send_bytes(uint32_t usart_periph, uint8_t *data, uint16_t len);
uint16_t uart_receive_bytes(uint32_t usart_periph, uint8_t *data, uint16_t len, uint16_t timeout);

// USART0 接收缓冲区（C2 二进制帧 + U2P 文本帧共用）
extern uint8_t uart0_rx_buf[512];
extern volatile uint16_t uart0_rx_len;

// USART2 接收缓冲区（预留）
extern uint8_t uart2_rx_buf[512];
extern volatile uint16_t uart2_rx_len;

#endif