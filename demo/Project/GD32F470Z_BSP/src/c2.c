#include <string.h>
#include "uart.h"
#include "c2.h"

// C2 插在主板槽位 1，物理连接 USART0 (PA9/PA10)
// c2_init 从 uart0_rx_buf ISR 缓冲区读响应

uint8_t c2_init(uint8_t zigbee_mode)
{
    uint8_t recv[50];
    uint8_t ok = 0;
    uint8_t i;

    uint8_t read_dev[4] = {0xFE, 0x01, 0xFE, 0xFF};
    uint8_t terminal[5]      = {0xFD, 0x02, 0x01, 0x02, 0xFF};
    uint8_t coordinator[5]   = {0xFD, 0x02, 0x01, 0x00, 0xFF};
    uint8_t pan_id[6]        = {0xFD, 0x03, 0x03, 0x2C, 0x3F, 0xFF};
    uint8_t group[5]         = {0xFD, 0x02, 0x09, 0x01, 0xFF};
    uint8_t key[20] = {0xFD,0x11,0x04,0x12,0x13,0x15,0x17,0x19,
                       0x1B,0x1D,0x1F,0x10,0x12,0x14,0x16,0x18,0x1A,0x1C,0x1D,0xFF};

    // Step1: 读设备
    ok = 0;
    for(i = 0; i < 3; i++) {
        memset(recv, 0, 50);
        uart_send_bytes(USART0, read_dev, 4);
        uart_receive_bytes(USART0, recv, 10, 200);
        if(recv[0] == 0xFB) { ok = 1; break; }
    }
    if(!ok) return 0;

    // Step2: 设模式
    ok = 0;
    for(i = 0; i < 3; i++) {
        memset(recv, 0, 50);
        uart_send_bytes(USART0, zigbee_mode == COORDINATOR ? coordinator : terminal, 5);
        uart_receive_bytes(USART0, recv, 10, 200);
        if(recv[0] == 0xFA && recv[1] == 0x01) { ok = 1; break; }
    }
    if(!ok) return 0;

    // Step3: PAN_ID
    ok = 0;
    for(i = 0; i < 3; i++) {
        memset(recv, 0, 50);
        uart_send_bytes(USART0, pan_id, 6);
        uart_receive_bytes(USART0, recv, 10, 200);
        if(recv[0] == 0xFA && recv[1] == 0x03) { ok = 1; break; }
    }
    if(!ok) return 0;

    // Step4: 网络组
    ok = 0;
    for(i = 0; i < 3; i++) {
        memset(recv, 0, 50);
        uart_send_bytes(USART0, group, 5);
        uart_receive_bytes(USART0, recv, 10, 200);
        if(recv[0] == 0xFA && recv[1] == 0x09) { ok = 1; break; }
    }
    if(!ok) return 0;

    // Step5: 密钥
    ok = 0;
    for(i = 0; i < 3; i++) {
        memset(recv, 0, 50);
        uart_send_bytes(USART0, key, 20);
        uart_receive_bytes(USART0, recv, 10, 200);
        if(recv[0] == 0xFA && recv[1] == 0x04) { ok = 1; break; }
    }
    return ok;
}

void c2_broadcast_data(char *data, uint8_t mode)
{
    char buf[100];
    uint8_t len = (uint8_t)strlen(data);
    buf[0] = 0xFC;
    buf[1] = len + 2;
    buf[2] = 0x01;
    buf[3] = mode;
    memcpy(&buf[4], data, len);
    uart_send_bytes(USART0, (uint8_t*)buf, len + 4);
}

uint8_t c2_rec_data(uint8_t *data, uint16_t len, uint16_t timeout)
{
    return uart_receive_bytes(USART0, data, len, timeout);
}
