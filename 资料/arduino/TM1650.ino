//written by JY-MCU,QQ:179383020
//Contact:QQ:179383020,Website:http://jy-mcu.taobao.com
#include <util/delay.h>
unsigned char SCL_pin=12;  //for ATMEGA8/168P/328P
unsigned char SDA_pin=13;
unsigned char tab[]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F,0x77,0x7C,0x39,0x5E,0x79,0x71};
void TM1650_start(void)
{
	pinMode(SDA_pin,OUTPUT);	//set SDA_pin output
	digitalWrite(SCL_pin,HIGH);
	digitalWrite(SDA_pin,HIGH);
	_delay_us(1);
	digitalWrite(SDA_pin,LOW);
	_delay_us(1);
}
void TM1650_stop(void)
{
	pinMode(SDA_pin,OUTPUT);
	digitalWrite(SCL_pin,HIGH);
	digitalWrite(SDA_pin,LOW);
	_delay_us(1);
	digitalWrite(SDA_pin,HIGH);
	_delay_us(1);
}
void TM1650_ACK(void)
{
	digitalWrite(SCL_pin,HIGH);
	pinMode(SDA_pin,INPUT);			//set SDA_pin input
	_delay_us(1);
	digitalWrite(SCL_pin,LOW);
	_delay_us(1);
	pinMode(SDA_pin,OUTPUT);
	_delay_us(1);
}
void TM1650_Write(unsigned char	DATA)			//写数据函数
{
	unsigned char i;
	pinMode(SDA_pin,OUTPUT);
	_delay_us(1);
	digitalWrite(SCL_pin,LOW);
	for(i=0;i<8;i++)
	{
		if(DATA&0X80)
			digitalWrite(SDA_pin,HIGH);
		else
			digitalWrite(SDA_pin,LOW);
		DATA<<=1;
		digitalWrite(SCL_pin,LOW);
		_delay_us(1);
		digitalWrite(SCL_pin,HIGH);
		_delay_us(1);
		digitalWrite(SCL_pin,LOW);
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

void port_init(void)
{
	pinMode(SCL_pin,OUTPUT);
	pinMode(SDA_pin,OUTPUT);
}

void setup()
{
	pinMode(SCL_pin,OUTPUT);
	pinMode(SDA_pin,OUTPUT);
	Write_DATA(0x48,0x31);
}
void loop()
{  
  unsigned char i,j;	
  for(i=0;i<16;i++)
		{
			for(j=0;j<4;j++)
				{
					Write_DATA(0x68+2*j,tab[(j+i)%16]|0x80);
				}
			_delay_ms(500);
		}
}
