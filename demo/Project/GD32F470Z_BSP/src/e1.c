#include "i2c.h"
#include <stdio.h>
#include "e1.h"


//0-F
const uint8_t disdata[19][2] ={		 {0xF8,0x01},
									 {0x30,0x00},
									 {0xD8,0x02},
									 {0x78,0x02},
									 {0x30,0x03},
									 {0x68,0x03},
									 {0xE8,0x03},
									 {0x38,0x00},
									 {0xF8,0x03},
									 {0x78,0x03},
									 {0xB8,0x03},
									 {0xE0,0x03},
									 {0xC8,0x01},
									 {0xF0,0x02},
									 {0xC8,0x03},
									 {0x88,0x03},
									 {0x00,0x00},
									 {0x88,0x02},  //额外新增字母R
									 {0xB0,0x01}  //额外新增字母N
														 };
															 
															 
															 
/*********************************************************************************************************
������:     get_e1_address
��ڲ���:   e1��ص�ַ(pca9685����htk1633��ַ)
���ڲ���:   �� 
����ֵ:     e1_addressʵ�ʵõ��Ľṹ���ַ
����:       zzz
����:       2023/3/30
��������:   �õ�pca9685��htk1633��i2c��ַ,����ʼ����ӦоƬ
**********************************************************************************************************/
i2c_addr_def get_e1_address(uint8_t address)
{
	i2c_addr_def e1_addess;

	e1_addess.flag = 0;

	if(i2c_addr_poll(I2C0,address))
	{
		e1_addess.periph = I2C0;
		e1_addess.addr = address;
		e1_addess.flag = 1;			
	} 			 
	if(e1_addess.flag != 1)
	{
		if(i2c_addr_poll(I2C1,address))
		{
			e1_addess.periph = I2C1;
			e1_addess.addr = address;
			e1_addess.flag = 1; 
		}						
	}	

	if(e1_addess.flag)
	{
		if( e1_addess.addr <= (PCA9685_ADDRESS_E1+8)  )
			pca9685_e1_init(e1_addess.periph,e1_addess.addr);	
		else if(e1_addess.addr >= HT16K33_ADDRESS_E1)
			ht16k33_e1_init(e1_addess.periph,e1_addess.addr);	
	} 		 

	return e1_addess;		 
}  


/*********************************************************************************************************
������:     e1_init
��ڲ���:   e1��ַ 
���ڲ���:   �� 
����ֵ:     i2c_addr_def����ṹ��
����:       zzz
����:       2023/3/28
��������:   �õ�pca9685��htk1633��i2c��ַ,��������������ṹ��flagֵΪ0
**********************************************************************************************************/
i2c_addr_def e1_init(uint8_t address)
{
	i2c_addr_def e1_addess;
	uint8_t i;

	e1_addess.flag = 0;

	if(address >= 0xC0 && address <= 0xE0)
	{
		for(i=0;i<4;i++)
		{
			if(i2c_addr_poll(I2C0,address+i*2))
			{
				e1_addess.periph = I2C0;
				e1_addess.addr = address+i*2;
				e1_addess.flag = 1;			
				break;
			} 
		}
		if(e1_addess.flag != 1)
		{			
			for(i=0;i<4;i++)
			{
				if(i2c_addr_poll(I2C1,address+i*2))
				{
					e1_addess.periph = I2C1;
					e1_addess.addr = address+i*2;
					e1_addess.flag = 1;
					break;
				}	
			}
		}

		if(e1_addess.flag)
		{
			if( e1_addess.addr <= (PCA9685_ADDRESS_E1+8)  )
				pca9685_e1_init(e1_addess.periph,e1_addess.addr);	
			else if(e1_addess.addr >= HT16K33_ADDRESS_E1)
				ht16k33_e1_init(e1_addess.periph,e1_addess.addr);	
		}	
	}			
	return e1_addess;	
}


/*********************************************************************************************************
������:     e1_all_init
��ڲ���:     e1_rgb_addr-pca9685��ʼ��ַ  e1_tube_addr-htk1633��ʼ��ַ
���ڲ���:   e1_address�ṹ��ָ��
����ֵ:     ��
����:       zzz
����:       2023/3/30
��������:   ���Ҳ���ʼ������E1�Ӱ�,���õ���Ӧ�ĵ�ַ���ڽṹ��ָ��e1_address
**********************************************************************************************************/
void e1_all_init(out e1_addr_def *e1_address,uint8_t e1_rgb_addr,uint8_t e1_tube_addr)
{
	uint8_t i;

	for(i=0;i<4;i++)
	{
		e1_address->rgb_addr[i] = get_e1_address(e1_rgb_addr+i*2);	
		e1_address->tube_addr[i] = get_e1_address(e1_tube_addr+i*2);				 
	}
}

/*********************************************************************************************************
������:     set_e1_pca9685_pwm_off
��ڲ���:   i2c_periph I2C��ѡ�� i2c0 or i2c1  i2c_addr ������ַ  
���ڲ���:   �� 
����:       zzz
����:       2023/3/28
��������:   �ر�pca9685����pwm�����
**********************************************************************************************************/
void set_e1_pca9685_pwm_off(uint32_t i2c_periph,uint8_t i2c_addr) 
{
    i2c_byte_write(i2c_periph,i2c_addr,ALLLED_ON_L,0);
    i2c_byte_write(i2c_periph,i2c_addr,ALLLED_ON_H,0);
    i2c_byte_write(i2c_periph,i2c_addr,ALLLED_OFF_L,0);
    i2c_byte_write(i2c_periph,i2c_addr,ALLLED_OFF_H,0x10);
}


/*********************************************************************************************************
������:     pc9685_e1_init
��ڲ���:   i2c_periph I2C��ѡ�� i2c0 or i2c1  i2c_addr ������ַ  
���ڲ���:   �� 
����:       zzz
����:       2023/3/28
��������:   ��ʼ��pca9685����оƬ
***********************************************************************************************************/
void pca9685_e1_init(uint32_t i2c_periph,uint8_t i2c_addr)
{
	i2c_delay_byte_write(i2c_periph,i2c_addr,E1_PCA9685_MODE1,0x0);
	set_e1_pca9685_pwm_off(i2c_periph,i2c_addr);   
}


/*********************************************************************************************************
������:     set_e1_pca9685_pwm
��ڲ���:   i2c_periph I2C��ѡ�� i2c0 or i2c1  i2c_addr ������ַ  
            num PCA9685��Ӧ������� on���ʹ�ܼ���ֵ off����رռ���ֵ��0-15��
���ڲ���:   �� 
����:       zzz
����:       2023/3/28
��������:   pwm duty = (off - on)/4095   ͨ��on��off��ֵ����ռ�ձ�
***********************************************************************************************************/
void set_e1_pca9685_pwm(uint32_t i2c_periph,uint8_t i2c_addr,uint8_t num, uint16_t on, uint16_t off) 
{
    i2c_byte_write(i2c_periph,i2c_addr,LED0_ON_L+4*num,on);
    i2c_byte_write(i2c_periph,i2c_addr,LED0_ON_H+4*num,on>>8);
    i2c_byte_write(i2c_periph,i2c_addr,LED0_OFF_L+4*num,off);
    i2c_byte_write(i2c_periph,i2c_addr,LED0_OFF_H+4*num,off>>8);
}


/*********************************************************************************************************
������:    set_e1_pca9685_frequency
��ڲ���:  i2c_periph I2C��ѡ�� i2c0 or i2c1  i2c_addr ������ַ  frequency �趨��Ƶ��ֵ 
���ڲ���:  �� 
����:      zzz
����:      2023/3/28
��������:  ����PWMƵ��
prescale value = round(25MHz/(4096*frequency)) - 1 example 30 = (25MHz/(4096*200)) - 1
maximum PWM frequency = 1526 Hz  prescale value = 0x03
minimum PWM frequency = 24 Hz prescale value = 0xff
***********************************************************************************************************/
void set_e1_pca9685_frequency(uint32_t i2c_periph,uint8_t i2c_addr,uint16_t frequency)
{
	uint8_t prescale_value;
	uint8_t old_mode,new_mode;

	if(frequency > 1526)
		frequency = 1526;	 
	else if(frequency < 24)
		frequency = 24;

	i2c_read(i2c_periph,i2c_addr,E1_PCA9685_MODE1,&old_mode,1);
	new_mode = (old_mode & 0x7F) | 0x10;
	i2c_byte_write(i2c_periph,i2c_addr,E1_PCA9685_MODE1,new_mode);
	prescale_value = (25*1000000/4096/frequency) - 1;
	i2c_byte_write(i2c_periph,i2c_addr,E1_PCA9685_PRESCALE,prescale_value);
	i2c_byte_write(i2c_periph,i2c_addr,E1_PCA9685_MODE1,old_mode);

	for(uint8_t i=0;i<20;i++)				//��ʱ
	{
		for(uint32_t j=0;j<100000;j++)
		{
		}	
	}

	i2c_byte_write(i2c_periph,i2c_addr,E1_PCA9685_MODE1,old_mode|0xa1);
}


/*********************************************************************************************************
������:    e1_rgb_control
��ڲ���:  i2c_periph I2C��ѡ�� i2c0 or i2c1  i2c_addr ������ַ  red:0-255 green:0-255 blue:0-255  (0-255)��Ӧռ�ձ�0-100%  
���ڲ���:  �� 
����:      zzz
����:      2023/3/28
����:      �ֱ������������ռ�ձ�,�����ɫ������
***********************************************************************************************************/
void e1_rgb_control(uint32_t i2c_periph,uint8_t i2c_addr,uint8_t red,uint8_t green,uint8_t blue)
{	
	  uint16_t off,on;
      
	  on = 0x0f;
	  off = on + red*0x10;    //red*0xfff/0xff
	  set_e1_pca9685_pwm(i2c_periph,i2c_addr,1,on,off);          //red
	   
	  on = 0x0f;
	  off = on + green*0x10;
	  set_e1_pca9685_pwm(i2c_periph,i2c_addr,0,on,off);	
	
	  on = 0x0f;
	  off = on + blue*0x10;
	  set_e1_pca9685_pwm(i2c_periph,i2c_addr,2,on,off);
}


/*********************************************************************************************************
������:     ht16k33_e1_display_off
��ڲ���:   i2c_periph I2C��ѡ�� i2c0 or i2c1  i2c_addr ������ַ  
���ڲ���:   �� 
����:       zzz
����:       2023/3/28
��������:   �ر��������ʾ
***********************************************************************************************************/
void ht16k33_e1_display_off(uint32_t i2c_periph,uint8_t i2c_addr)
{
	i2c_byte_write(i2c_periph,i2c_addr,0x02,0x00);
	i2c_byte_write(i2c_periph,i2c_addr,0x03,0x00);
	i2c_byte_write(i2c_periph,i2c_addr,0x04,0x00);
	i2c_byte_write(i2c_periph,i2c_addr,0x05,0x00);
	i2c_byte_write(i2c_periph,i2c_addr,0x06,0x00);
	i2c_byte_write(i2c_periph,i2c_addr,0x07,0x00);
	i2c_byte_write(i2c_periph,i2c_addr,0x08,0x00);
	i2c_byte_write(i2c_periph,i2c_addr,0x09,0x00);
	i2c_cmd_write(i2c_periph,i2c_addr,DISPLAY_ON);
}


/*********************************************************************************************************
������:     ht16k33_e1_init
��ڲ���:   i2c_periph I2C��ѡ�� i2c0 or i2c1  i2c_addr ������ַ  
���ڲ���:   �� 
����:       zzz
����:       2023/3/28
��������:   ��ʼ�������оƬ����ht16k33,���ر���ʾ
***********************************************************************************************************/
void ht16k33_e1_init(uint32_t i2c_periph,uint8_t i2c_addr)
{
	i2c_cmd_write(i2c_periph,i2c_addr,SYSTEM_ON);
	ht16k33_e1_display_off(i2c_periph,i2c_addr);
}


/*********************************************************************************************************
������:     ht16k33_e1_display_data
��ڲ���:   i2c_periph I2C��ѡ�� i2c0 or i2c1  i2c_addr ������ַ bit ��ʾλ��  dataҪ��ʾ���� 
���ڲ���:   �� 
����:       zzz
����:       2023/3/28
��������:   ��ʾ4������ܵ�����0-F 0x10����ʾ
***********************************************************************************************************/
void ht16k33_e1_display_data(uint32_t i2c_periph,uint8_t i2c_addr,uint8_t bit,uint8_t data)
{
	switch(bit)
	{
		case  1:
		i2c_byte_write(i2c_periph,i2c_addr,0x02,disdata[data][0]);
		i2c_byte_write(i2c_periph,i2c_addr,0x03,disdata[data][1]);
		break;

		case  2:
		i2c_byte_write(i2c_periph,i2c_addr,0x04,disdata[data][0]);
		i2c_byte_write(i2c_periph,i2c_addr,0x05,disdata[data][1]);
		break;

		case 3:
		i2c_byte_write(i2c_periph,i2c_addr,0x06,disdata[data][0]);
		i2c_byte_write(i2c_periph,i2c_addr,0x07,disdata[data][1]);
		break;

		case 4:
		i2c_byte_write(i2c_periph,i2c_addr,0x08,disdata[data][0]);
		i2c_byte_write(i2c_periph,i2c_addr,0x09,disdata[data][1]);
		break;

	}
	i2c_cmd_write(i2c_periph,i2c_addr,DISPLAY_ON);
}


/*********************************************************************************************************
������:     e1_digital_display
��ڲ���:   i2c_periph I2C��ѡ�� i2c0 or i2c1  i2c_addr ������ַ 
            dis1 dis2 dis3 dis4��Ӧ�����1-4λ�õ���ʾ����0-F 
���ڲ���:   �� 
����:       zzz
����:       2023/3/28
��������:   ��ʾ�����1-4λ�����Ӧ������(0-F) 0x10����ʾ
***********************************************************************************************************/
void e1_digital_display(uint32_t i2c_periph,uint8_t i2c_addr,uint8_t dis1,uint8_t dis2,uint8_t dis3,uint8_t dis4)
{
	ht16k33_e1_display_data(i2c_periph,i2c_addr,1,dis1);
	ht16k33_e1_display_data(i2c_periph,i2c_addr,2,dis2);
	ht16k33_e1_display_data(i2c_periph,i2c_addr,3,dis3);
	ht16k33_e1_display_data(i2c_periph,i2c_addr,4,dis4);
}
