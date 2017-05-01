//written by JY-MCU,QQ:179383020

//Contact:QQ:179383020,Website:http://jy-mcu.taobao.com
#ifndef	_TM1650_H
#define	_TM1650_H

#include	<avr/io.h>
#include	<util/delay.h>

#define	SDA_COMMAND	0X40
#define	DISP_COMMAND	0x80
#define	ADDR_COMMAND	0XC0

#define SET_BIT(PORT,BIT)	PORT|=(1<<BIT)
#define	CLR_BIT(PORT,BIT)	PORT&=~(1<<BIT)
#define	BIT_IN(DDR,BIT)		DDR&=~(1<<BIT)
#define	BIT_OUT(DDR,BIT)	DDR|=(1<<BIT)
#define	READ_BIT(PIN,BIT)	(PIN&(1<<BIT))

#define	SDA					PD7
#define	SCL					PD6

#define SDA_high			SET_BIT(PORTD,SDA)
#define SDA_low				CLR_BIT(PORTD,SDA)
#define SCL_high			SET_BIT(PORTD,SCL)
#define SCL_low				CLR_BIT(PORTD,SCL)

#define SDA_IN				BIT_IN(DDRD,SDA)	//输入状态
#define SDA_OUT				BIT_OUT(DDRD,SDA)	//输出状态
#define	SDA_READ			READ_BIT(PIND,SDA)	//读引脚电平

unsigned char tab[]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F,0x77,0x7C,0x39,0x5E,0x79,0x71};
void TM1650_start(void)
{
	SDA_OUT;	//set SDA output
	SCL_high;
	SDA_high;
	_delay_us(1);
	SDA_low;
	_delay_us(1);
}
void TM1650_stop(void)
{
	SDA_OUT;
	SCL_high;
	SDA_low;
	_delay_us(1);
	SDA_high;
	_delay_us(1);
}
void TM1650_ACK(void)
{
	SCL_high;
	SDA_IN;			//set SDA input
	_delay_us(1);
	SCL_low;
	_delay_us(1);
	SDA_OUT;
	_delay_us(1);
}
void TM1650_Write(unsigned char	DATA)			//写数据函数
{
	unsigned char i;
	SDA_OUT;
	_delay_us(1);
	SCL_low;
	for(i=0;i<8;i++)
	{
		if(DATA&0X80)
			SDA_high;
		else
			SDA_low;
		DATA<<=1;
		SCL_low;
		_delay_us(1);
		SCL_high;
		_delay_us(1);
		SCL_low;
		_delay_us(1);
	}	
}
void Write_DATA(unsigned char add,unsigned char DATA)		//指定地址写入数据
{
	TM1650_start();
	TM1650_Write(add);
	TM1650_ACK();
	TM1650_Write(DATA);
	TM1650_ACK();
	TM1650_stop();
}/*
void init_TM1650(void)
{
	Write_DATA(0x48,0x21);
	Write_DATA(0x68,0xaa);
}*/
#endif
