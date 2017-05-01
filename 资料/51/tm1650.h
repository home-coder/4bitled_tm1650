//written by JY-MCU,QQ:179383020

//Contact:QQ:179383020,Website:http://jy-mcu.taobao.com
#ifndef	_TM1650_H
#define	_TM1650_H

sbit SDA=P1^0;
sbit SCL=P1^1;

void _delay_us(unsigned char i)
{
	while(i--);
}

unsigned char tab[]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F,0x77,0x7C,0x39,0x5E,0x79,0x71};
void TM1650_start(void)
{
	SCL=1;
	SDA=1;
	_delay_us(1);
	SDA=0;
	_delay_us(1);
}
void TM1650_stop(void)
{
	SCL=1;
	SDA=0;
	_delay_us(1);
	SDA=1;
	_delay_us(1);
}
void TM1650_ACK(void)
{
	SCL=1;
	SDA=1;			//set SDA input
	_delay_us(1);
	SCL=0;
	_delay_us(1);
}
void TM1650_Write(unsigned char	DATA)			//写数据函数
{
	unsigned char i;
	_delay_us(1);
	SCL=0;
	for(i=0;i<8;i++)
	{
		if(DATA&0X80)
			SDA=1;
		else
			SDA=0;
		DATA<<=1;
		SCL=0;
		_delay_us(1);
		SCL=1;
		_delay_us(1);
		SCL=0;
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
}
#endif
