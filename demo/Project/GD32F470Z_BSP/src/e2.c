#include "i2c.h"
#include "e2.h"



/*********************************************************************************************************
函数名:     get_e2_address
入口参数:   e2初始地址
出口参数:   无 
返回值:     eborad_address实际得到的结构体地址
作者:       zzz
日期:       2023/3/31
调用描述:   得到pca9685 i2c地址,并初始化对应芯片
**********************************************************************************************************/
i2c_addr_def get_e2_address(uint8_t address)
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
     
     if(eboard_addess.flag)
		 pca9685_e2_init(eboard_addess.periph,eboard_addess.addr);	
			 		 
	 return eboard_addess;		 
} 


/*********************************************************************************************************
函数名:     set_e2_pca9685_pwm_off
入口参数:   i2c_periph I2C口选择 i2c0 or i2c1  i2c_addr 器件地址  
出口参数:   无 
作者:       zzz
日期:       2023/3/28
调用描述:   关闭pca9685所有pwm输出口
**********************************************************************************************************/
void set_e2_pca9685_pwm_off(uint32_t i2c_periph,uint8_t i2c_addr) 
{
    i2c_byte_write(i2c_periph,i2c_addr,ALLCN_ON_L,0);
    i2c_byte_write(i2c_periph,i2c_addr,ALLCN_ON_H,0);
    i2c_byte_write(i2c_periph,i2c_addr,ALLCN_OFF_L,0);
    i2c_byte_write(i2c_periph,i2c_addr,ALLCN_OFF_H,0x10);
}


/*********************************************************************************************************
函数名:     pc9685_e2_init
入口参数:   i2c_periph I2C口选择 i2c0 or i2c1  i2c_addr 器件地址  
出口参数:   无 
作者:       zzz
日期:       2023/3/28
调用描述:   初始化pca9685驱动芯片
***********************************************************************************************************/
void pca9685_e2_init(uint32_t i2c_periph,uint8_t i2c_addr)
{
	i2c_delay_byte_write(i2c_periph,i2c_addr,E2_PCA9685_MODE1,0x0); //模式初始化
	set_e2_pca9685_pwm_off(i2c_periph,i2c_addr);   
}


/*********************************************************************************************************
函数名:     set_e2_pca9685_pwm
入口参数:   i2c_periph I2C口选择 i2c0 or i2c1  i2c_addr 器件地址  
            num PCA9685对应的输出口 on输出使能计数值 off输出关闭计数值（0-15）
出口参数:   无 
作者:       zzz
日期:       2023/3/28
调用描述:   pwm duty = (off - on)/4095   通过on和off的值设置占空比
***********************************************************************************************************/
void set_e2_pca9685_pwm(uint32_t i2c_periph,uint8_t i2c_addr,uint8_t num, uint16_t on, uint16_t off) 
{

    i2c_delay_byte_write(i2c_periph,i2c_addr,CN0_ON_L+4*num,on);
    i2c_delay_byte_write(i2c_periph,i2c_addr,CN0_ON_H+4*num,on>>8);
    i2c_delay_byte_write(i2c_periph,i2c_addr,CN0_OFF_L+4*num,off);
    i2c_delay_byte_write(i2c_periph,i2c_addr,CN0_OFF_H+4*num,off>>8);
}


/*********************************************************************************************************
函数名:     e2_init
入口参数:   e2地址 
出口参数:   无 
返回值:     i2c_addr_def定义结构体
作者:       zzz
日期:       2023/3/29
调用描述:   得到e2 pca9685 i2c地址,若器件不存在则结构体flag值为0,若存在则初始化芯片置结构体flag值为1
**********************************************************************************************************/
i2c_addr_def e2_init(uint8_t address)
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
		pca9685_e2_init(e_addess.periph,e_addess.addr);	

	return e_addess;		 
}


/*********************************************************************************************************
函数名:     e2_all_init
入口参数:   e2_address结构体指针  e2_addr-pca9685起始地址 
出口参数:   e2_address结构体指针
返回值:     无
作者:       zzz
日期:       2023/3/31
调用描述:   查找并初始化所有E2子板,并得到相应的地址放在结构体指针e2_address
**********************************************************************************************************/
void e2_all_init(out e2_addr_def *e2_address,uint8_t e2_addr)
{
	uint8_t i;

	for(i=0;i<4;i++)
	{
		e2_address->motor_addr[i] = get_e2_address(e2_addr+i*2);	
	}
}



/*********************************************************************************************
函数名:     e2_speed_control
入口参数:   e2 i2c口 e2地址, speed_level转速百分比(0-100)
出口参数:   无 
返回值:     无
作者:       zzz
日期:       2023/3/29
调用描述:   控制风扇转速
**********************************************************************************************/
void e2_speed_control(uint32_t i2c_periph,uint8_t i2c_addr,uint8_t speed_level)
{
	uint16_t off,on;

	if(speed_level > 100)
		speed_level = 100;

	on = 0x00;
	off = on + 0xfff*speed_level/100;
	set_e2_pca9685_pwm(i2c_periph,i2c_addr,0,on,off);
}



















