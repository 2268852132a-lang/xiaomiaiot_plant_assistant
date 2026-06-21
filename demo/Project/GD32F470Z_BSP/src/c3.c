#include "uart.h"
#include "systick.h"
#include "stdio.h"
#include "string.h"
#include "c3.h"

// ==================== C3 调试输出辅助 ====================
// 将调试信息通过 C3 自身 UART 回传到调试串口不方便，
// 这里改为通过 uart_send_bytes + 返回值字符串由主程序打印

/**********************************************************************************************
C3 UART 底层收发（已由 uart.c 实现）
***********************************************************************************************/
int Uart_write(char *sendbuf, int len)
{
	return uart_send_bytes(USART5,(uint8_t *)sendbuf,len);
}

int Uart_read(out char *recvbuf, int len, int timeout)
{
	return uart_receive_bytes(USART5, (uint8_t *)recvbuf, len, timeout);
}



/*********************************************************************************************
??????:    c3_init
????:      ???WIFI???AT???????????
??????:  ??
????????? ??
???????   AT????????????1  ?????????????0
?????     ZZZ
????:      2023/4/6
????????:  ?��?C3?????????
**********************************************************************************************/
uint8_t c3_init(char *debug_out)
{
		int i;
		int recv_cnt = 0;
		char recv_buf[64] = {0};
		
		// C3 ESP-AT 模块上电需要 2-3 秒初始化，先等待
		delay_ms(3000);
		
		for(i = 0; i < 5; i++)             //尝试5次发送AT指令
		{
				Uart_write("AT\r\n", strlen("AT\r\n"));    //发送AT测试指令
				recv_cnt = Uart_read(recv_buf, sizeof(recv_buf) - 1, 200);
				if(recv_cnt > 0) recv_buf[recv_cnt] = '\0';
				if(strstr(recv_buf,"OK") != NULL)
				{
					  if(debug_out) sprintf(debug_out, "C3 OK (try %d): %s", i+1, recv_buf);
					  return 1;
				}
				if(debug_out) sprintf(debug_out, "C3 try %d: %s", i+1, 
				                      recv_cnt > 0 ? recv_buf : "(no response)");
		}
		return 0;
}




/*********************************************************************************************
??????:    at_cmd
????:      ????wifi at????
??????:  *send_data????????????  *str_cmd ????????  wait_time:??????
????????? ??
???????   ???????????1  ?????????????0
?????     ZZZ
????:      2023/4/8
????????:  ?????????at????,?��??????????
**********************************************************************************************/
uint8_t at_cmd(char *send_data,char *str_cmd,uint16_t wait_time)
{
	int	recv_cnt = 0;
	char	recv_buf[80] = {0};

	if(Uart_write(send_data, strlen(send_data)) <= 0)
		return 0;

	recv_cnt = Uart_read(recv_buf, sizeof(recv_buf), wait_time);
	if(recv_cnt <= 0)
		return 0;

	if(strstr(recv_buf,str_cmd) != NULL)
		return 1;
	else
		return 0;	 
}


/******************************************************************************************************************
??????:    c3_wifi_net_status_get
????:      ????????????
??????:  *recv_buf ???????????
????????? ??
???????   wifi???????-1 ????????-3 ????????0
?????     ZZZ
????:      2023/4/8
????????:  ?????????????��???
 *****************************************************************************************************************/
int c3_wifi_net_status_get(char *recv_buf)
{
		//???????????
		char *rec = strstr(recv_buf,"ERROR");
		if (rec != NULL)
		{
				//printf("C3 ERROR\n");
		}

		rec = strstr(recv_buf,"+IPD,");
		if(rec == NULL)
		{
				rec = strstr(recv_buf,"CLOSED");
				if(rec != NULL)
				{
					return -1;
				}
		}

		rec = strstr(recv_buf,"ready\r\n");
		if(rec != NULL)
		{
				return -3;
		}
		
		return 0;
}



/*********************************************************************************************
??????:    c3_wifi_tcp_init
????:      ?????c3 wifi????
??????:  wifi_run_state ?????????
????????? *rec_data 
???????   
?????     ZZZ
????:      2023/4/8
????????:  ?????????at????,?????WIFI???
**********************************************************************************************/
int c3_wifi_tcp_init(int wifi_run_state,out char *rec_data)
{
	int		recv_cnt;
	char	recv_buf[80] = {0};

	switch(wifi_run_state)
	{
		case 1:	//??????
		{
			if(Uart_write("ATE0\r\n", strlen("ATE0\r\n")) <= 0)
			{
				goto WIFI_ERR;
			}
			//???????????
			recv_cnt = Uart_read(recv_buf, sizeof(recv_buf), 300);

			if(recv_cnt <= 0)
			{
				goto WIFI_ERR;
			}
			if(strstr(recv_buf,"OK") != NULL)
			{
				goto WIFI_OK;
			}
			break;
		}
		case 2:	//??????
		{
			if(Uart_write("AT+CWMODE=1\r\n", strlen("AT+CWMODE=1\r\n")) <= 0)
			{
				goto WIFI_ERR;
			}
			//???????????
			recv_cnt = Uart_read(recv_buf, sizeof(recv_buf), 300);

			if(recv_cnt <= 0)
			{
				goto WIFI_ERR;
			}
			if(strstr(recv_buf,"OK") != NULL)
			{
				goto WIFI_OK;
			}
			break;
		}
		case 3://????????????
		{
			recv_cnt = 0;
			delay_ms(1000);
			if(Uart_write("AT+CWJAP?\r\n", strlen("AT+CWJAP?\r\n")) <= 0)
			{
				goto WIFI_ERR;
			}
			//???????????
			recv_cnt = Uart_read(recv_buf, sizeof(recv_buf), 300);

			char *rec = strstr(recv_buf,"+CWJAP");
			if(rec != NULL)
			{
				memcpy(rec_data,recv_buf,recv_cnt);
				return 1;  //??????????  ????????????
			}
			if(recv_cnt <= 0)
			{
				goto WIFI_ERR;
			}
			if(strstr(recv_buf,"OK") != NULL)
			{
				goto WIFI_OK;
			}
			break;
		}
		case 4://????WIFI???
		{
			recv_cnt = 0;

			if(Uart_write("AT+CWJAP=\"" WIFI_SSID "\",\"" WIFI_PASSWORD "\"\r\n", strlen("AT+CWJAP=\"" WIFI_SSID "\",\"" WIFI_PASSWORD "\"\r\n")) <= 0)
			{
				goto WIFI_ERR;
			}
			//???????????
			for(int x = 10;x != 0; x --)
			{
				recv_cnt += Uart_read(&recv_buf[recv_cnt], sizeof(recv_buf) - recv_cnt, 100);
				if(strstr(recv_buf,"ERROR") != NULL)
				{
					break;
				}
				if(strstr(recv_buf,"OK") != NULL)
				{
					goto WIFI_OK;
				}
			}
			goto WIFI_ERR;

		}
		case 5://???????????????
		{
			recv_cnt = 0;
			if(Uart_write("AT+CIFSR\r\n", strlen("AT+CIFSR\r\n")) <= 0)
			{
				goto WIFI_ERR;
			}
			//???????????
			recv_cnt = Uart_read(&recv_buf[recv_cnt], sizeof(recv_buf) - recv_cnt, 300);
			if(recv_cnt <= 0)
			{
				break;
			}
			char *rec = strstr(recv_buf,"+CIFSR:STAIP,\"0.0.0.0\"");
			if(rec != NULL)
			{
				goto WIFI_ERR;
			}
			if(strstr(recv_buf,"OK") != NULL)
			{
				goto WIFI_OK;
			}
			break;
		}
		case 6://????????IP????
		{
			recv_cnt = 0;
			if(Uart_write("AT+CIPSTART=\"TCP\",\"" SERVER_IP "\"," SERVER_PORT "\r\n", strlen("AT+CIPSTART=\"TCP\",\"" SERVER_IP "\"," SERVER_PORT "\r\n")) <= 0)
			{
				goto WIFI_ERR;
			}
			//???????????
			for(int x = 10;x != 0; x --)
			{
				recv_cnt += Uart_read(&recv_buf[recv_cnt], (sizeof(recv_buf) - recv_cnt), 100);
				if(strstr(recv_buf,"ERROR") != NULL)
				{
					goto WIFI_ERR;
				}
				if(strstr(recv_buf,"CONNECT") != NULL)
				{
					//break;
					goto WIFI_OK;
				}
				if(strstr(recv_buf,"OK") != NULL)
				{
					goto WIFI_OK;
				}
			}
			break;
		}
		case  8:
		{
			Uart_write("AT+RST\r\n", strlen("AT+RST\r\n"));
			delay_ms(3000);
			return -2;
		}	
		default:
		break;
	}
	WIFI_ERR:
	memcpy(rec_data,recv_buf,recv_cnt);
	return -1;

	WIFI_OK:
	memcpy(rec_data,recv_buf,recv_cnt);
	return 0;
}



/*********************************************************************************************
??????:    c3_wifi_tcp_lead
????:      ?????c3 wifi????
??????:  ??
????????? *wifi_run_state ????????? *tcp_status:tcp??
???????   ??
?????     ZZZ
????:      2023/4/8
????????:  ????AT??????wifi?????
**********************************************************************************************/
void c3_wifi_tcp_lead(out int *wifi_run_state,out int *tcp_status, char *debug_out)
{
    char rec_buf[80]={0};
	int net_state;
  
    if(*tcp_status == -1)	
	{
		int wifi_rec = 0;
		wifi_rec = c3_wifi_tcp_init(*wifi_run_state,rec_buf);        //执行一步状态机
		if(debug_out) rec_buf[sizeof(rec_buf)-1] = '\0';
		
		if(wifi_rec == 0)
		{
			if(*wifi_run_state == 6)
			{
				*tcp_status = 0;
				*wifi_run_state = 1;
				if(debug_out) sprintf(debug_out, "Step%d OK -> TCP connected", 6);
			}
			else
			{
				if(debug_out) sprintf(debug_out, "Step%d OK -> Step%d", *wifi_run_state, *wifi_run_state+1);
				*wifi_run_state = *wifi_run_state + 1;
			}
		}
		else if(wifi_rec == 1)
		{
			*wifi_run_state = 5;
			if(debug_out) sprintf(debug_out, "Step%d: already connected, jump to 5", *wifi_run_state);
		}
		else if(wifi_rec == -1)
		{
			*wifi_run_state = 8;
			if(debug_out) sprintf(debug_out, "Step%d FAIL: %s", *wifi_run_state, rec_buf);
		}		
		else if(wifi_rec == -2)
		{
			*wifi_run_state = 1;
			if(debug_out) sprintf(debug_out, "Reset -> restart from 1");
		}		

		net_state = c3_wifi_net_status_get(rec_buf);
		if(net_state == -1)
			*wifi_run_state = 8;
		else if(net_state == -3)
			*wifi_run_state = 1;					
	}
}



/*********************************************************************************************
??????:    c3_wifi_tcp_send
????:      c3 wifi????????
??????:  *send_data ???????????
????????? ??
???????   ???????1,??????0
?????     ZZZ
????:      2023/4/8
????????:  ????wifi????
**********************************************************************************************/
uint8_t c3_wifi_tcp_send(char *send_data)
{
	uint8_t len;
	char send_buffer[200];
	char recv_buf[80] = {0};

	len = strlen(send_data);
	memset(send_buffer,0,200);
	sprintf(send_buffer,"AT+CIPSEND=%d\r\n",len);
	Uart_write(send_buffer, strlen(send_buffer));
	delay_ms(200);
	memset(send_buffer,0,200);
	strncpy(send_buffer,(const char*)send_data,len);	
	Uart_write(send_buffer,len);

	Uart_read(recv_buf, sizeof(recv_buf), 300);
	if(strstr(recv_buf,"SEND OK") != NULL)
		len = 1;
	else
		len = 0;

	return len;
}


/*********************************************************************************************
??????:    c3_wifi_tcp_receive
????:      ????wifi?��?????
??????:  wait_time:??????
????????? *rec_data:???????????  
???????   ?????????
?????     ZZZ
????:      2023/4/8
????????:  ????wifi?��?????
**********************************************************************************************/
uint16_t c3_wifi_tcp_receive(out char *rec_data,uint16_t wait_time)
{
	uint16_t rec_len;	

	rec_len = Uart_read(rec_data,300,wait_time);

	return rec_len;
}


