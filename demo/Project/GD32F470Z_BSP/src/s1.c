#include "i2c.h"
#include "s1.h"

/*********************************************************************************************************
函数名:     s1_init
输入参数:   i2c初始化地址 address 
输出参数:   无 
返回值:     i2c_addr_def地址结构体
作者:       zzz
时间:       2023/4/1
函数功能:   得到s1 i2c地址,如果正确返回结构体flag值为0,如果错误初始化对应芯片结构体flag值为1
**********************************************************************************************************/
i2c_addr_def s1_init(uint8_t address)
{
	i2c_addr_def e_addess;
	uint8_t i;

	e_addess.flag = 0;

	for(i=0;i<4;i++)
	{
		if(i2c_addr_poll(I2C0,address+i*2))
		{
			e_addess.periph = I2C0;
			e_addess.addr = address+i*2;
			e_addess.flag = 1;			
			break;
		} 
	}
	if(e_addess.flag != 1)
	{			
		for(i=0;i<4;i++)
		{
			if(i2c_addr_poll(I2C1,address+i*2))
			{
				e_addess.periph = I2C1;
				e_addess.addr = address+i*2;
				e_addess.flag = 1;
				break;
			}	
		}
	}

	if(e_addess.flag)
	{
		i2c_cmd_write(e_addess.periph,e_addess.addr,S1_SYSTEM_ON);
	}
	return e_addess;		 
}


/*********************************************************************************************************
函数名:     s1_all_init
输入参数:   s1_address结构体指针  s1_addr:s1 i2c初始化地址 
输出参数:   s1_address结构体指针
返回值:     无
作者:       zzz
时间:       2023/4/1
函数功能:   依次初始化所有s1子板,得到对应地址放到结构体指针s1_address
**********************************************************************************************************/
void s1_all_init(out s1_addr_def *s1_address,uint8_t s1_addr)
{
	uint8_t i;

	for(i=0;i<4;i++)
	{
		s1_address->key_addr[i] = s1_init(s1_addr+i*2);
	}
}


/*********************************************************************************************
函数名:     s1_key_scan
功能:       按键扫描函数,得到键值
输入参数:   i2c_periph:i2c选择  i2c_addr:i2c初始化地址
输出参数:   无
返回值:     返回按键扫描值
作者:       ZZZ
时间:       2023/4/1
函数功能:   使用此函数,得到键值
**********************************************************************************************/
uint8_t s1_key_scan(uint32_t i2c_periph,uint8_t i2c_addr)
{
	uint8_t key_value;
	uint8_t keyvalue[6];

	i2c_read(i2c_periph,i2c_addr,KEYKS0,keyvalue,6);

	// 调试：打印原始按键数据
	if(keyvalue[0] != 0 || keyvalue[2] != 0 || keyvalue[4] != 0) {
		// 我们没有 printf，所以这里先不打印，用 main.c 里的调试
	}

	if(keyvalue[0]&0x01)
	{
		key_value = SW1;
	}	
	else if(keyvalue[2]&0x01)
	{	
		key_value = SW2;	 
	}
	else if(keyvalue[4]&0x01)
	{
		key_value = SW3;
	}	
	else if(keyvalue[0]&0x02)
	{
		key_value = SW4;
	}	
	else if(keyvalue[2]&0x02)
	{
		key_value = SW5; 
	}	
	else if(keyvalue[4]&0x02)
	{
		key_value = SW6; 
	}
	else if(keyvalue[0]&0x04)
	{
		key_value = SW7;
	}
	else if(keyvalue[2]&0x04)
	{
		key_value = SW8;
	}
	else if(keyvalue[4]&0x04)
	{
		key_value = SW9;
	}
	else if(keyvalue[0]&0x08)
	{
		key_value = SWA;
	}
	else if(keyvalue[2]&0x08)
	{
		key_value = SW0;
	}
	else if(keyvalue[4]&0x08)
	{
		key_value = SWC;
	}	
	else 
	{
		key_value = SWN;
	}	

	return key_value;		 
}
