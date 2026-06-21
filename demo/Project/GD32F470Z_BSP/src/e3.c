#include "i2c.h"
#include "e3.h"

/*********************************************************************************************************
函数名:     get_e3_address
入口参数:   e3初始地址
出口参数:   无 
返回值:     eborad_address实际得到的结构体地址
作者:       zzz
日期:       2023/3/31
调用描述:   得到窗帘电机i2c地址,并初始化对应芯片
**********************************************************************************************************/
i2c_addr_def get_e3_address(uint8_t address)
{
	i2c_addr_def eboard_addess;

	eboard_addess.flag = 0;

	if(i2c_addr_poll(I2C0,address))
	{
		eboard_addess.periph = I2C0;
		eboard_addess.addr = address;
		eboard_addess.flag = 1;			
	} 			 
	if(eboard_addess.flag != 1)
	{
		if(i2c_addr_poll(I2C1,address))
		{
			eboard_addess.periph = I2C1;
			eboard_addess.addr = address;
			eboard_addess.flag = 1; 
		}						
	}	

	return eboard_addess;		 
} 




/*********************************************************************************************************
函数名:     e3_init
入口参数:   e3地址 address 
出口参数:   无 
返回值:     i2c_addr_def定义结构体
作者:       zzz
日期:       2023/3/29
调用描述:   得到e3 i2c地址,若器件不存在则结构体flag值为0,若存在则初始化芯片置结构体flag值为1
**********************************************************************************************************/
i2c_addr_def e3_init(uint8_t address)
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

	return e_addess;		 
}


/*********************************************************************************************************
函数名:     e3_all_init
入口参数:   e3_address结构体指针  e3_addr-窗帘电机起始地址 
出口参数:   e3_address结构体指针
返回值:     无
作者:       zzz
日期:       2023/3/31
调用描述:   查找并初始化所有E3子板,并得到相应的地址放在结构体指针e3_address
**********************************************************************************************************/
void e3_all_init(out e3_addr_def *e3_address,uint8_t e3_addr)
{
	   uint8_t i;
	  
	   for(i=0;i<4;i++)
       {
			e3_address->curtain_addr[i] = get_e3_address(e3_addr+i*2);
	   }
}

/*********************************************************************************************************
函数名:     e3_set_position
入口参数:   i2c_periph:i2c口   i2c_addr:e3 i2c地址, re_pos 位置百分比(0-100)
出口参数:   无 
返回值:     i2c_addr_def定义结构体
作者:       zzz
日期:       2023/3/29
调用描述:   设置窗帘电机位置
**********************************************************************************************************/
void e3_set_position(uint32_t i2c_periph,uint8_t i2c_addr,uint8_t re_pos)
{
	  i2c_delay_byte_write(i2c_periph,i2c_addr,0x03,re_pos);
}




