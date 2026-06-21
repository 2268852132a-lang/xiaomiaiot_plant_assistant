#include "uart.h"

#define  out

#define WIFI_SSID 		"otto"
#define WIFI_PASSWORD 	"sjqnb666"

// Server酱 (https://sct.ftqq.com) 微信推送
#define SEND_KEY        "SCT364734T675QUzjoTZVCwI90SFdPYHQD"
#define SERVER_IP       "sctapi.ftqq.com"
#define SERVER_PORT     "80"


uint8_t c3_init(char *debug_out);
int c3_wifi_tcp_init(int wifi_run_state,char *rec_data);
void c3_wifi_tcp_lead(int *wifi_run_state,int *tcp_status, char *debug_out);
uint8_t c3_wifi_tcp_send(char *send_data);
uint16_t c3_wifi_tcp_receive(char *rec_data,uint16_t wait_time);
